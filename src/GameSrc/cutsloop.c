
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

uiSlab cutscene_slab;
LGRegion cutscene_root_region;

int current_cutscene;
bool should_show_credits;

uchar cutscene_key_handler(uiEvent *ev, LGRegion *r, void *user_data) {
	uiCookedKeyEvent *kev = (uiCookedKeyEvent *)ev;
    int code = kev->code & ~(KB_FLAG_DOWN | KB_FLAG_2ND);

    if (kev->code & KB_FLAG_DOWN) {
    	switch(code) {
		case KEY_ESC:
		case KEY_ENTER:
		case KEY_SPACE:
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

	return 1;
}