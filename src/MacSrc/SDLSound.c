
#include <Carbon/Carbon.h>
#include "musicai.h"
#include "adlmidi.h"

#ifdef USE_SDL_MIXER

#include <SDL_mixer.h>

static Mix_Chunk *samples_by_channel[SND_MAX_SAMPLES];
static snd_digi_parms digi_parms_by_channel[SND_MAX_SAMPLES];

#define SAMPLE_RATE 44100
#define SAMPLES 4096

struct ADL_MIDIPlayer *adlDevice;

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

short adl_buffer[SAMPLES * 2];
int is_playing = 0;

static void SDL_MidiAudioCallback(void *adl_midi_player, Uint8 *stream, int len)
{
	struct ADL_MIDIPlayer* p = (struct ADL_MIDIPlayer*)adl_midi_player;

    /* Convert bytes length into total count of samples in all channels */
    int samples_count = len / 2;

    /* Take some samples from the ADLMIDI */
    samples_count = adl_play(p, samples_count, (short*)adl_buffer);

    if(samples_count <= 0)
    {
        is_playing = 0;
        SDL_memset(stream, 0, len);
        return;
    }

    /* Send buffer to the audio device */
    SDL_memcpy(stream, (Uint8*)adl_buffer, samples_count * 2);
}

int MacTuneLoadTheme(char* theme_base, int themeID) {
	char filename[30];

	// Try to play some music! theme_base will be a string like 'thm0'

	// FIXME: This should really try to play the .xmi files in res/sound/genmidi if there is a way!
	// until then, I'm going to just attempt to play .mid files instead.

	// Build the file name
	strcpy(filename, "res/sound/genmidi/");
	strcat(filename, theme_base - 0);
	strcat(filename, ".xmi");

	DEBUG("Playing music %s", filename);

	if(adlDevice != NULL) {
		SDL_CloseAudio();
	}

	adlDevice = adl_init(SAMPLE_RATE);

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

    // Bank 45 is System Shock?
    adl_setBank(adlDevice, 45);

    if(SDL_OpenAudio(&spec, &obtained) < 0) {
    	ERROR("Could not open audio for music!\n\n");
    }

    adl_switchEmulator(adlDevice, 1);
    adl_setLoopEnabled(adlDevice, 1);
    adl_setNumChips(adlDevice, 4);
    adl_setVolumeRangeModel(adlDevice, 1);

    if(adl_openFile(adlDevice, filename) != 0)
    {
    	ERROR("Could not open music file for ADL");
    }

    for(int i = 0; i < 64; i++) {
		adl_setTrackOptions(adlDevice, i, ADLMIDI_TrackOption_Off);    	
    }

    adl_setTrackOptions(adlDevice, 0, ADLMIDI_TrackOption_On);
    adl_setTrackOptions(adlDevice, 9, ADLMIDI_TrackOption_On);

    SDL_PauseAudio(0);

	/*Mix_Music *music;
	music = Mix_LoadMUS(filename);
	Mix_PlayMusic(music, -1);*/
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
