
#include <Carbon/Carbon.h>
#include "musicai.h"
#include "adlmidi.h"
#include "mlimbs.h"

#ifdef USE_SDL_MIXER

#include <SDL_mixer.h>

static Mix_Chunk *samples_by_channel[SND_MAX_SAMPLES];
static snd_digi_parms digi_parms_by_channel[SND_MAX_SAMPLES];

#define SAMPLE_RATE 44100
#define SAMPLES 4096
#define CHANNELS MLIMBS_MAX_CHANNELS

struct ADL_MIDIPlayer *adlDevice[MLIMBS_MAX_CHANNELS];

int snd_start_digital(void) {

	// Startup the sound system

	if(Mix_Init(MIX_INIT_MP3) < 0) {
		DebugString("SDL_Mixer: Init failed");
	}

	if(Mix_OpenAudio(SAMPLE_RATE, MIX_DEFAULT_FORMAT, 2, SAMPLES / 2) < 0) {
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
	Mix_VolumeMusic((curr_vol_lev * curr_vol_lev) * 128 / 10000);
}

int is_playing = 0;

static void SDL_MidiAudioCallback(void *adl_midi_player, Uint8 *stream, int len)
{
    /* Convert bytes length into total count of samples in all channels */
    int samples_count = len / 2;
    SDL_memset(stream, 0, len);

    /* Take some samples from the ADLMIDI */
    for(int i = 0; i < CHANNELS; i++) {
    	int read = 0;
    	if(adlDevice[i] != NULL) {
    		short adl_buffer[SAMPLES * 2];
    		read = adl_play(adlDevice[i], samples_count, (short*)adl_buffer);
	    	for(int x = 0; x < read * 2; x++) {
	    		stream[x] += ((Uint8*)adl_buffer)[x];
	    	}
    	}
	}

    //mlimbs_callback(NULL, 1);
    mlimbs_timer_callback();
}

int MacTuneLoadTheme(char* theme_base, int themeID) {
	char music_filename[30];
	char data_filename[30];
	char bin_filename[30];

	// Try to play some music! theme_base will be a string like 'thm0'

	// FIXME: This should really try to play the .xmi files in res/sound/genmidi if there is a way!
	// until then, I'm going to just attempt to play .mid files instead.

	// Build the file name
	strcpy(music_filename, "res/sound/genmidi/");
	strcat(music_filename, theme_base - 0);
	strcat(music_filename, ".xmi");

	// Build the data file name
	strcpy(data_filename, "res/sound/");
	strcat(data_filename, theme_base - 0);
	strcat(data_filename, ".dat");

	// Build the tbin file name
	strcpy(bin_filename, "res/sound/");
	strcat(bin_filename, theme_base - 0);
	strcat(bin_filename, ".bin");

	DEBUG("Playing music %s", music_filename);

	FILE* bin_file = fopen(bin_filename, "rb");
	if(bin_file != NULL) {
		extern uchar track_table[NUM_SCORES][SUPERCHUNKS_PER_SCORE]; 
		extern uchar transition_table[NUM_TRANSITIONS];
		extern uchar key_table[NUM_LAYERABLE_SUPERCHUNKS][KEY_BAR_RESOLUTION];
		extern uchar layering_table[NUM_LAYERS][MAX_KEYS];

		// Read in the Music AI stuff
		fread(track_table, sizeof(uchar), NUM_SCORES * SUPERCHUNKS_PER_SCORE, bin_file);
		fread(layering_table, sizeof(uchar), NUM_LAYERS * MAX_KEYS, bin_file);
		fread(transition_table, sizeof(uchar), NUM_TRANSITIONS, bin_file);
		fread(key_table, sizeof(uchar), NUM_LAYERABLE_SUPERCHUNKS * KEY_BAR_RESOLUTION, bin_file);
		fclose(bin_file);
	}

	SDL_CloseAudio();
	for(int i = 0; i < CHANNELS; i++) {
		if(adlDevice[i] != NULL) {
			adl_close(adlDevice[i]);
			adlDevice[i] = NULL;
		}
	}

	for(int i = 0; i < CHANNELS; i++) {
		struct ADL_MIDIPlayer *adlD = adl_init(SAMPLE_RATE);

		// Bank 45 is System Shock?
		adl_setBank(adlD, 45);
		adl_switchEmulator(adlD, 1);
	    adl_setLoopEnabled(adlD, 1);
	    adl_setNumChips(adlD, 1);
	    adl_setVolumeRangeModel(adlD, 1);

	    adl_openFile(adlD, music_filename);
    	for(int x = 0; x < 64; x++) {
			adl_setTrackOptions(adlD, 0, ADLMIDI_TrackOption_Solo);
	    }

	    adlDevice[i] = adlD;
	}

	if(adlDevice == NULL) {
		ERROR("Could not open ADLMIDI");
	}

    SDL_AudioSpec spec;
    SDL_AudioSpec obtained;

    spec.freq = SAMPLE_RATE;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = SAMPLES;

    spec.callback = SDL_MidiAudioCallback;
    spec.userdata = adlDevice;

    if(SDL_OpenAudio(&spec, &obtained) < 0) {
    	ERROR("Could not open audio for music!\n\n");
    }

	//adl_setTrackOptions(adlDevice[0], 0, ADLMIDI_TrackOption_Solo);

    SDL_PauseAudio(0);

	/*Mix_Music *music;
	music = Mix_LoadMUS(filename);
	Mix_PlayMusic(music, -1);*/
	MacTuneUpdateVolume();

	mlimbs_init();
	int r = mlimbs_load_theme(music_filename, data_filename, themeID);
	if(r > 0) mlimbs_start_theme();
	DEBUG("mlimbs_load_theme: %i", r);

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
