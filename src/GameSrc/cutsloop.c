
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

uiSlab cutscene_slab;
LGRegion cutscene_root_region;

int current_cutscene;
bool should_show_credits;

int cutscene_id = -1;
int cutscene_idx;
int cutscene_len;
ActAnim *main_anim = NULL;

Afile *amovie;

char* cutscene_files[3] = {
	"res/data/svgaintr.res",
	"res/data/death.res", 
	"res/data/win1.res"
};

char* cutscene_music[3] = {
	"Intro",
	"dead",
	"enda"
};

Ref cutscene_anims[3] = {
	0xbd6,
	0x1e,
	0x1d4
};

Ref cutscene_anims_len[3] = {
	19,
	7,
	3
};

Ref cutscene_pals[5][20] = {
 {5,6,7,8,9,9,10,10,11,11,11,12,13,13,14,14,15,16,17,18},
 {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
 {18,19,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20},
};

int current_cutscene = -1;
void cutscene_anim_end(ActAnim *paa, AnimCode ancode, AnimCodeData *pdata)
{
	cutscene_id = -1;
	cutscene_idx++;

	// Go back to the main menu if we're done
	if(cutscene_idx >= cutscene_len) {
	    _new_mode = SETUP_LOOP;
		chg_set_flg(GL_CHG_LOOP);

		if(should_show_credits) {
			journey_credits_func(FALSE);
		}

		uiShowMouse(NULL);
	}
}

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

	if(amovie != NULL) {
	    fix time;
	    Apalette pal;

	    grs_bitmap bitmap;
	    bitmap.bits = NULL;

	    AfileReadFullFrame(amovie, &bitmap, &time);

	    if(AfileGetFramePal(amovie, &pal)) {
	    	DEBUG("Setting pal");
	    	gr_set_pal(pal.index, pal.numcols, pal.rgb);
	    }

	    gr_bitmap(&bitmap, 0, 0);

	    return;
	}

	fix sint, cost;
	fix_sincos(*tmd_ticks * 50, &sint, &cost);
	int ymov = fix_int(fix_mul(sint, fix_make(5, 0)));

	char buff[100];
	sprintf(buff, "Cutscene #%i should go here!", current_cutscene);

	switch(current_cutscene) {
		case DEATH_CUTSCENE:
			res_draw_text(RES_coloraliasedFont, "Game Over, Hacker.", 30, 70 - ymov);
			break;
		case WIN_CUTSCENE:
			res_draw_text(RES_coloraliasedFont, "It's over. Shodan is dust.", 30, 70 - ymov);
			break;
		case START_CUTSCENE:
			res_draw_text(RES_coloraliasedFont, "Welcome back to Citadel Station", 30, 70 - ymov);
			break;
		default:
			res_draw_text(RES_coloraliasedFont, buff, 50, 70 - ymov);
	}
	
	res_draw_text(RES_coloraliasedFont, "[ Press space to continue ]", 30, 90 - ymov);
}

short play_cutscene(int id, bool show_credits) {
	INFO("Playing Cutscene %i", id);

	_new_mode = CUTSCENE_LOOP;
	chg_set_flg(GL_CHG_LOOP);

	int cf = ResOpenFile(cutscene_files[id]);
	if(cf <= 0)
		return 0;

	amovie = malloc(sizeof(Afile));
    if (AfilePrepareRes(cutscene_anims[id], amovie) < 0) {
        WARN("%s: Cannot open Afile by id $%x", __FUNCTION__, cutscene_anims[id]);
        free(amovie);
        return (ERR_FREAD);
    }

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

	return 1;
}