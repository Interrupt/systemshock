
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
Apalette pal;
grs_bitmap movie_bitmap;
long next_time;

bool is_first_frame;
bool done_playing_movie;
long start_time;
long next_draw_time;

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

void CaptureMouse(bool capture);

void cutscene_start() {
	DEBUG("Cutscene start");

	#ifdef SVGA_SUPPORT
    	extern void change_svga_screen_mode();
    	change_svga_screen_mode();
	#endif

	generic_reg_init(TRUE, &cutscene_root_region, NULL, &cutscene_slab, cutscene_key_handler, cutscene_mouse_handler);

	_current_view = &cutscene_root_region;
	uiSetCurrentSlab(&cutscene_slab);

	uiHideMouse(NULL);

	CaptureMouse(FALSE);
}

void cutscene_exit() {
	DEBUG("Cutscene exit");
	MacTuneKillCurrentTheme();
}

//filled in amov.c when chunk contains subtitle data
extern char EngSubtitle[256];
extern char FrnSubtitle[256];
extern char GerSubtitle[256];

void cutscene_loop() {

    fix time;
    long cur_time = SDL_GetTicks();
    static uint8_t palette[3*256];

    if(is_first_frame) {
    	is_first_frame = FALSE;
    	next_time = cur_time;
    	start_time = SDL_GetTicks();

    	// Read the first frame
    	AfileReadFullFrame(amovie, &movie_bitmap, &time);
    	next_draw_time = start_time + fix_float(time) * 1000.0;

    	// Set the initial palette
        memcpy(palette, amovie->v.pal.rgb, 3*256);
    	gr_clear(0x00);
    }

    if(cur_time > next_draw_time) {
    	// Get the palette for this frame, if any
    	if(AfileGetFramePal(amovie, &pal)) {
            memcpy(palette+3*pal.index, pal.rgb, 3*pal.numcols);
	    }

        //not really a hack: find unused color in movie bitmap and use it as subtitle color
        {
            uint32_t used[256], used_min = 0xFFFFFFFF;
            memset(used, 0, sizeof(used));
            uint8_t *bits = movie_bitmap.bits;
            int count = (int)movie_bitmap.w * movie_bitmap.h;
            while (count--) used[*bits++]++;
            int i, used_min_i = 1;
            for (i=1; i<256-1; i++)
                if (used[i] < used_min) {used_min = used[i]; used_min_i = i;}
            uint8_t temp_pal[3*256];
            memcpy(temp_pal, palette, 3*256);
            for (i=0; i<3; i++) temp_pal[3*used_min_i+i] = temp_pal[3*255+i];
            gr_set_pal(0, 256, temp_pal);
            gr_set_fcolor(used_min_i);
        }

	    // Draw this frame
	    gr_clear(0x00);

	    float vscale = (float)amovie->v.height / (float)amovie->v.width;

	    int offset = (320 - (amovie->v.width / 2)) / 2;
		ss_scale_bitmap(&movie_bitmap, offset, offset / 1.25, amovie->v.width / 2, amovie->v.height / 2);

        //draw subtitles
        char *buf = 0;
        extern char which_lang;
        switch (which_lang)
        {
            case 0: default: buf = EngSubtitle; break;
            case 1: buf = FrnSubtitle; break;
            case 2: buf = GerSubtitle; break;
        }
        if (buf && *buf)
        {
            short w, h, x, y;
            grs_font *fon = gr_get_font();
            gr_set_font((grs_font *)ResLock(RES_cutsceneFont));
            gr_string_size(buf, &w, &h);
            x = (320-w)/2;
            y = 158+(200-158-h)/2;
            ss_string(buf, x, y);
            ResUnlock(RES_cutsceneFont);
            gr_set_font(fon);
        }

		if(done_playing_movie) {
			// Go back to the main menu
            _new_mode = SETUP_LOOP;
			chg_set_flg(GL_CHG_LOOP);

			if(should_show_credits) {
				journey_credits_func(FALSE);
			}

			return;
		}

		// Read the next frame
    	if(AfileReadFullFrame(amovie, &movie_bitmap, &time) == -1) {
    		DEBUG("Done playing movie!");
    		done_playing_movie = TRUE;

    		// Still want a bit of a delay before finishing
    		next_draw_time += 100;
    	}
    	else {
    		next_draw_time = start_time + fix_float(time) * 1000.0;
    	}
    }
}

short play_cutscene(int id, bool show_credits) {
	INFO("Playing Cutscene %i", id);

    *EngSubtitle = 0;
    *FrnSubtitle = 0;
    *GerSubtitle = 0;

	_new_mode = CUTSCENE_LOOP;
	chg_set_flg(GL_CHG_LOOP);

	// Set some initial variables
	is_first_frame = TRUE;
    done_playing_movie = FALSE;
	movie_bitmap.bits = NULL;

	// Open the movie file
	int cf = ResOpenFile(cutscene_files[id]);
	if(cf <= 0)
		return 0;

	amovie = malloc(sizeof(Afile));
    if (AfilePrepareRes(cutscene_anims[id], amovie) < 0) {
        WARN("%s: Cannot open Afile by id $%x", __FUNCTION__, cutscene_anims[id]);
        free(amovie);
        return (ERR_FREAD);
    }

    // Now start playing the audio for this movie
    int32_t audio_length = AfileAudioLength(amovie) * MOVIE_DEFAULT_BLOCKLEN;
    uint8_t *buffer = malloc(audio_length);
    AfileGetAudio(amovie, buffer);
    
    snd_digi_parms *sdp = malloc(sizeof(snd_digi_parms));
    sdp->vol = 128;
    sdp->pan = 64;
    sdp->snd_ref = 0;
    sdp->data = NULL;

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
    extern uchar sfx_on;
    if (sfx_on) snd_alog_play(0x0, cvt.len_cvt, cvt.buf, sdp);

    // Reset the movie reader
    AfileReadReset(amovie);

	return 1;
}
