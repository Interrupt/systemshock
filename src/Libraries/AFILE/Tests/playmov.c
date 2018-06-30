//
// Created by winterheart on 17.06.18.
//

#include <SDL.h>
#include <SDL_mixer.h>

#include "afile.h"
#include "movie.h"

int main(int argc, char *argv[]) {

    Afile *afile = malloc(sizeof(Afile));;
    MovieHeader mh;
    int32_t audio_length, bitmap_length;
    MovieChunk *pmc, *pmcBase;
    char *infile;

    char *chunk;

    Mix_Chunk *_sample[2];


    if (argc < 2) {
        printf("Usage:  movinfo file.mov\n");
        exit(1);
    }
    infile = argv[1];

    if (AfileOpen(afile, infile) < 0) {
        printf("Can't open: %s\n", infile);
        exit(1);
    }
    bitmap_length = AfileBitmapLength(afile);
    audio_length = AfileAudioLength(afile) * 8192;

}