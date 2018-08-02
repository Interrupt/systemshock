
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

uiSlab cutscene_slab;
LGRegion cutscene_root_region;

int current_cutscene;
bool should_show_credits;

int cutscene_id = -1;
int cutscene_idx;
int cutscene_len;
ActAnim *main_anim = NULL;

char* cutscene_files[3] = {
	"res/data/start1.res",
	"res/data/death.res",
	"res/data/win1.res"
};

char* cutscene_music[3] = {
	"Intro",
	"dead",
	"enda"
};

Ref cutscene_anims[3] = {
	0x1bc,
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
}

void cutscene_loop() {
	gr_clear(0xFF);

	if(cutscene_len > 0) {
		if(cutscene_id == -1) {
			LGPoint animloc;
			animloc.x = 0;
			animloc.y = 25;

			uchar* cpal = (uchar*)ResLock(cutscene_pals[current_cutscene][cutscene_idx]);
			if(cpal != NULL)
				gr_set_pal(0, 256, cpal);

			cutscene_id = cutscene_idx;
			main_anim = AnimPlayRegion(MKREF(cutscene_anims[current_cutscene] + cutscene_idx, 0), root_region, animloc, 0, NULL);
			AnimSetNotify(main_anim, NULL, ANCODE_KILL, cutscene_anim_end);

			cutscene_id = cutscene_idx;
		}

		AnimRecur();
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

	current_cutscene = id;
	should_show_credits = show_credits;
	main_anim = NULL;

	cutscene_idx = 0;
	cutscene_len = cutscene_anims_len[id];
	cutscene_id = -1;

	if(cutscene_len > 0) {
		int cp = ResOpenFile("res/data/cutspal.res");
		if(cp <= 0)
			return 0;

		int cf = ResOpenFile(cutscene_files[id]);
		if(cf <= 0)
			return 0;

		MacTuneKillCurrentTheme();
		uiHideMouse(NULL);

		MacTuneLoadTheme("intro", 0);
	}

	return 1;
}