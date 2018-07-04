//
// Created by winterheart on 17.06.18.
//

#include <SDL.h>
#include <SDL_mixer.h>
#include <afile.h>

#include "afile.h"
#include "movie.h"

// FIXME workaround for bitmap.c hardcode
int32_t gScreenRowbytes = 1024;

int main(int argc, char *argv[]) {

    Afile *afile = malloc(sizeof(Afile));
    int32_t audio_length, bitmap_length;
    char *infile;
    char *buffer;

    log_set_quiet(0);
    log_set_level(LOG_TRACE);

    if (argc < 2) {
        printf("Usage:  movinfo file.mov\n");
        return 1;
    }
    infile = argv[1];

    if (AfileOpen(afile, infile) < 0) {
        ERROR("Can't open: %s", infile);
        return 1;
    }

    printf("File: %s\n", infile);
    bitmap_length = AfileBitmapLength(afile);
    audio_length = AfileAudioLength(afile) * 8192;

    buffer = malloc(audio_length);
    AfileGetAudio(afile, buffer);

    if(Mix_Init(MIX_INIT_MP3) < 0) {
        DEBUG("SDL_Mixer: Init failed");
    }

    if(Mix_OpenAudio(fix_int(afile->a.sampleRate), AUDIO_U8, afile->a.numChans, 8192) < 0) {
        DEBUG("SDL_Mixer: Couldn't open audio device");
    }
    Mix_AllocateChannels(SND_MAX_SAMPLES);

    if (SDL_Init(SDL_INIT_AUDIO)) {
        ERROR("Could not initialize SDL - %s", SDL_GetError());
        return 2;
    }

    Mix_Chunk *sample = Mix_QuickLoad_RAW(buffer, audio_length);

    int channel = Mix_PlayChannel(-1, sample, 0);

    if (channel < 0) {
        ERROR("SDL_Mixer: Failed to play sample");
        Mix_FreeChunk(sample);
        return 2;
    }
    DEBUG("Beginning to play");
    while(Mix_Playing(channel) != 0) { }
    DEBUG("Playing done");

    free(buffer);
    Mix_FreeChunk(sample);
    SDL_Quit();

    DEBUG("We hope you have a pleasant stay on Citadel Station");
    return 0;
}