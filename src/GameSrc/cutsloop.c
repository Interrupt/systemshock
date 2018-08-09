
/*
 * Cutscene loops
 */

#include "cutsloop.h"
#include "mainloop.h"
#include "game_screen.h"
#include "gamescr.h"
#include "tools.h"
#include "faketime.h"
#include "setploop.h"
#include "hkeyfunc.h"

#include "anim.h"
#include "statics.h"

#include "MacTune.h"
#include "gr2ss.h"

#include "afile.h"
#include "movie.h"

#include <SDL.h>

uiSlab cutscene_slab;
LGRegion cutscene_root_region;
bool should_show_credits;

Afile *amovie;
grs_bitmap movie_bitmap;
long next_time;

char* cutscene_files[3] = {
	"res/data/svgaintr.res",
	"res/data/svgadeth.res", 
	"res/data/svgaend.res"
};

Ref cutscene_anims[3] = {
	0xbd6,
	0xbd7,
	0xbd8
};

uchar cutscene_key_handler(uiEvent *ev, LGRegion *r, void *user_data) {
	uiCookedKeyEvent *kev = (uiCookedKeyEvent *)ev;
    int code = kev->code & ~(KB_FLAG_DOWN | KB_FLAG_2ND);

    if (kev->code & KB_FLAG_DOWN) {
    	switch(code) {
		case KEY_ESC:
		case KEY_ENTER:
		case KEY_SPACE:
			uiShowMouse(NULL);

			// Go back to the main menu
            _new_mode = SETUP_LOOP;
			chg_set_flg(GL_CHG_LOOP);

			if(should_show_credits) {
				journey_credits_func(FALSE);
			}

            break;
        }
    }
}

uchar cutscene_mouse_handler(uiEvent *ev, LGRegion *r, void *user_data) {

}

void cutscene_start() {
	DEBUG("Cutscene start");

	#ifdef SVGA_SUPPORT
    	extern void change_svga_screen_mode();
    	change_svga_screen_mode();
	#endif

	generic_reg_init(TRUE, &cutscene_root_region, NULL, &cutscene_slab, cutscene_key_handler, cutscene_mouse_handler);

	_current_view = &cutscene_root_region;
	uiSetCurrentSlab(&cutscene_slab);
}

void cutscene_exit() {
	DEBUG("Cutscene exit");
	MacTuneKillCurrentTheme();
}

void cutscene_loop() {
	gr_clear(0xFF);

    fix time;
    Apalette pal;

    long cur_time = SDL_GetTicks();
    long frame_rate = fix_int(amovie->v.frameRate);

    if(cur_time >= next_time) {
	    // Read the next frame
	    if(AfileReadFullFrame(amovie, &movie_bitmap, &time) != -1) {
	    	// Also get the next palette
		    if(AfileGetFramePal(amovie, &pal)) {
		    	gr_set_pal(pal.index, pal.numcols, pal.rgb);
		    }

		    int overflow = cur_time - next_time;
			next_time += (frame_rate * 10) - overflow;
		}
		else {
			// Go back to the main menu
            _new_mode = SETUP_LOOP;
			chg_set_flg(GL_CHG_LOOP);

			if(should_show_credits) {
				journey_credits_func(FALSE);
			}
		}
	}

	gr_bitmap(&movie_bitmap, 0, 0);
}

short play_cutscene(int id, bool show_credits) {
	INFO("Playing Cutscene %i", id);

	_new_mode = CUTSCENE_LOOP;
	chg_set_flg(GL_CHG_LOOP);

	movie_bitmap.bits = NULL;

	int cf = ResOpenFile(cutscene_files[id]);
	if(cf <= 0)
		return 0;

	amovie = malloc(sizeof(Afile));
    if (AfilePrepareRes(cutscene_anims[id], amovie) < 0) {
        WARN("%s: Cannot open Afile by id $%x", __FUNCTION__, cutscene_anims[id]);
        free(amovie);
        return (ERR_FREAD);
    }

    gr_set_pal(0, 256, amovie->v.pal.rgb);

    int32_t audio_length = AfileAudioLength(amovie) * MOVIE_DEFAULT_BLOCKLEN;
    uint8_t *buffer = malloc(audio_length);
    AfileGetAudio(amovie, buffer);
    
    snd_digi_parms *sdp = malloc(sizeof(snd_digi_parms));
    sdp->vol = 128;
    sdp->pan = 64;
    sdp->snd_ref = 0;

    // Need to convert the movie's audio format to ours
    // FIXME: Might want to use SDL_AudioStream to do this on the fly
    SDL_AudioCVT cvt;
    SDL_BuildAudioCVT(&cvt, AUDIO_U8, 1, fix_int(amovie->a.sampleRate), MIX_DEFAULT_FORMAT, 2, MIX_DEFAULT_FREQUENCY);
    cvt.len = audio_length;  // 1024 stereo float32 sample frames.
    cvt.buf = (Uint8 *) SDL_malloc(cvt.len * cvt.len_mult);

    SDL_memcpy(cvt.buf, buffer, audio_length); // copy our bytes to be converted
    SDL_ConvertAudio(&cvt); // cvt.buf will have cvt.len_cvt bytes of converted data after this

    // Stop current music
	MacTuneKillCurrentTheme();

	// Now play the sample
    snd_alog_play(0x0, cvt.len_cvt, cvt.buf, sdp);

    // Reset the movie reader
    AfileReadReset(amovie);

    next_time = SDL_GetTicks();

	return 1;
}