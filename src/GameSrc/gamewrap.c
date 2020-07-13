/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
/*
 * $Source: r:/prj/cit/src/RCS/gamewrap.c $
 * $Revision: 1.101 $
 * $Author: xemu $
 * $Date: 1994/11/26 03:36:36 $
 */

#include <string.h>

#include "Shock.h"

#include "amap.h"
#include "archiveformat.h"
#include "criterr.h"
#include "cybmem.h"
#include "drugs.h"
#include "dynmem.h"
#include "faketime.h"
#include "frprotox.h"
#include "gamewrap.h"
#include "hud.h"
#include "invent.h"
#include "invpages.h"
#include "leanmetr.h"
#include "mainloop.h"
#include "miscqvar.h"
#include "musicai.h" // to tweak music upon startup
#include "newmfd.h"
#include "objects.h"
#include "objload.h"
#include "objsim.h"
#include "olhext.h"
#include "player.h"
#include "saveload.h"
#include "schedule.h"
#include "setup.h"
#include "shodan.h"
#include "sideicon.h"
#include "status.h"
#include "tools.h"
#include "trigger.h"
#include "wares.h"
#include "wrapper.h"

#include "otrip.h"

#include <stdio.h>

#define SCHEDULE_BASE_ID 590

extern long old_ticks;
extern char saveload_string[30];
extern uchar display_saveload_checkpoints;
extern ulong obj_check_time;
extern uchar mlimbs_on;

// Player struct support for savegames.
// DOS version savegame reserves 32 bytes for puzzle state.
#define PL_MFD_PUZZLE_SIZE 32
#include "playerlayout.h"
#undef PL_MFD_PUZZLE_SIZE
// Enhanced edition uses 64.
#define PL_MFD_PUZZLE_SIZE 64
#include "playerlayout.h"
#undef PL_MFD_PUZZLE_SIZE

const ResLayout *PlayerLayouts[] = { &PlayerLayout_M32, &PlayerLayout_M64 };
// Decode wrapper for a player layout. Tries to figure out which version saved
// the game from the resource size.
void *decode_player(void *raw, size_t *size, UserDecodeData layout) {
    int i;
    for (i = 0; i < sizeof PlayerLayouts / sizeof *PlayerLayouts; ++i) {
	if (*size == PlayerLayouts[i]->dsize) {
	    return ResDecode(raw, size, (UserDecodeData)PlayerLayouts[i]);
	}
    }
    ERROR("Could not determine format of saved player!");
    return NULL;
}
// Player format. We always save as enhanced format (64-byte MFD array).
const ResourceFormat PlayerFormat = {
    decode_player, ResEncode, (UserDecodeData)&PlayerLayout_M64, NULL };
#define FORMAT_PLAYER (&PlayerFormat)

//-------------------
//  INTERNAL PROTOTYPES
//-------------------
errtype load_game_schedules(void);
errtype interpret_qvars(void);

#define OldResIdFromLevel(level) (OLD_SAVE_GAME_ID_BASE + (level * 2) + 2)

errtype copy_file(char *src_fname, char *dest_fname) {
    FILE *fsrc, *fdst;
    DEBUG("copy_file: %s to %s", src_fname, dest_fname);

    fsrc = fopen_caseless(src_fname, "rb");
    if (fsrc == NULL) {
        return ERR_FOPEN;
    }

    fdst = fopen_caseless(dest_fname, "wb");
    if (fdst == NULL) {
        return ERR_FOPEN;
    }

    int b;
    while ((b = fgetc(fsrc)) != EOF) {
        fputc(b, fdst);
    }

    fclose(fsrc);
    fclose(fdst);

    return OK;
}

void closedown_game(uchar visible) {
    // clear any transient hud settings
    hud_shutdown_lines();
    drug_closedown(visible);
    hardware_closedown(visible);
    musicai_clear();
    clear_digi_fx();
    olh_closedown();
    fr_closedown();
    if (visible)
        reset_schedules();
}

void startup_game(uchar visible) {
    drug_startup(visible);
    hardware_startup(visible);
    if (visible) {
        mfd_force_update();
        side_icon_expose_all();
        status_vitals_update(TRUE);
        inventory_page = 0;
        inv_last_page = INV_BLANK_PAGE;
    }
}

#ifdef NOT_YET

void check_save_game_wackiness(void) {
    // for now, the only thing we have heard of is a bridge in general inventory
    // so lets make sure geninv has only geninvable stuff
    int i;
    ObjID cur_test;
    for (i = 0; i < NUM_GENERAL_SLOTS; i++) {
        cur_test = player_struct.inventory[i];
#ifdef USELESS_OBJECT_CHECK
        if (cur_test != OBJ_NULL) {
            if ((ObjProps[OPNUM(cur_test)].flags & INVENTORY_GENERAL) == 0)
                Warning(("You have obj %d a %d as the %d element of geninv, BADNESS\n", cur_test, OPNUM(cur_test), i));
            //         else
            //            Warning(("You have obj %d a %d as the %d element of geninv ok
            //            %x\n",cur_test,OPNUM(cur_test),i,ObjProps[OPNUM(cur_test)].flags));
        }
#endif
    }
}

#endif // NOT_YET

errtype save_game(char *fname, char *comment) {
    int filenum;
    State player_state;
    errtype retval;
    int idx = SAVE_GAME_ID_BASE;

    // KLC - this does nothing now.		check_save_game_wackiness();
    // Why is this done???			closedown_game(FALSE);

    DEBUG("starting save_game");

    // KLC  do it the Mac way						i = flush_resource_cache();
    // Size	dummy;
    // MaxMem(&dummy); DG: I don't think this is needed anymore

    // Open the current game file to save some more resources into it.
    // FSMakeFSSpec(gDataVref, gDataDirID, CURRENT_GAME_FNAME, &currSpec);
    filenum = ResEditFile(CURRENT_GAME_FNAME, FALSE);
    if (filenum < 0) {
        ERROR("Couldn't open Current Game");
        return ERR_FOPEN;
    }

    // Sakeave comment
    ResMake(idx, (void *)comment, strlen(comment) + 1, RTYPE_APP, filenum, RDF_LZW, FORMAT_RAW);
    ResWrite(idx);
    ResUnmake(idx);
    idx++;

    // Save player struct (resource #4001)
    player_struct.version_num = PLAYER_VERSION_NUMBER;
    player_struct.realspace_loc = objs[player_struct.rep].loc;
    EDMS_get_state(objs[PLAYER_OBJ].info.ph, &player_state);
    LG_memcpy(player_struct.edms_state, &player_state, sizeof(fix) * 12);
    // LZW later		ResMake(idx, (void *)&player_struct, sizeof(player_struct), RTYPE_APP, filenum,
    // RDF_LZW);

    ResMake(idx, (void *)&player_struct, sizeof(player_struct), RTYPE_APP, filenum, 0, FORMAT_PLAYER);
    ResWrite(idx);
    ResUnmake(idx);
    idx++;

    // HAX HAX HAX Skip the schedule for now!
    // Save game schedule (resource #590)
    idx = SCHEDULE_BASE_ID;
    // LZW later		ResMake(idx, (void *)&game_seconds_schedule, sizeof(Schedule), RTYPE_APP, filenum,
    // RDF_LZW);

    ResMake(idx, (void *)&game_seconds_schedule, sizeof(Schedule), RTYPE_APP, filenum, 0, FORMAT_SCHEDULE);
    ResWrite(idx);
    ResUnmake(idx);
    idx++;

    // Save game schedule vec info (resource #591)
    // LZW later		ResMake(idx, (void *)game_seconds_schedule.queue.vec, sizeof(SchedEvent)*GAME_SCHEDULE_SIZE,
    // RTYPE_APP, filenum, RDF_LZW);
    ResMake(idx, (void *)game_seconds_schedule.queue.vec, sizeof(SchedEvent) * GAME_SCHEDULE_SIZE, RTYPE_APP, filenum,
            0, FORMAT_SCHEDULE_QUEUE);
    ResWrite(idx);
    ResUnmake(idx);
    idx++;

    ResCloseFile(filenum);

    // Save current level
    retval = write_level_to_disk(ResIdFromLevel(player_struct.level), TRUE);
    if (retval) {
        ERROR("Return value from write_level_to_disk is non-zero!"); //
        critical_error(CRITERR_FILE | 3);
    }

    // Copy current game out to save game slot
    if (copy_file(CURRENT_GAME_FNAME, fname) != OK) {
        // Put up some alert here.
        ERROR("No good copy, dude!");
        //		string_message_info(REF_STR_SaveGameFail);
    }
    // KLC	else
    // KLC		string_message_info(REF_STR_SaveGameSaved);
    old_ticks = *tmd_ticks;
    // do we have to do this?		startup_game(FALSE);
    return (OK);
}

errtype load_game_schedules(void) {
    char *oldvec;
    int idx = SCHEDULE_BASE_ID;

    oldvec = game_seconds_schedule.queue.vec;
    ResExtract(idx++, FORMAT_SCHEDULE, &game_seconds_schedule);
    game_seconds_schedule.queue.vec = oldvec;
    game_seconds_schedule.queue.comp = compare_events;
    ResExtract(idx++, FORMAT_SCHEDULE_QUEUE, oldvec);
    return OK;
}

errtype interpret_qvars(void) {
#ifdef SVGA_SUPPORT
    extern short mode_id;
#endif
    extern uchar fullscrn_vitals;
    extern uchar fullscrn_icons;
    extern uchar map_notes_on;
    extern ubyte hud_color_bank;

    // KLC - don't do this here - it's a global now.   load_da_palette();

    gamma_dealfunc(QUESTVAR_GET(GAMMACOR_QVAR));

    // dclick_dealfunc(QUESTVAR_GET(DCLICK_QVAR));
    // joysens_dealfunc(QUESTVAR_GET(JOYSENS_QVAR));

    recompute_music_level(QUESTVAR_GET(MUSIC_VOLUME_QVAR));
    recompute_digifx_level(QUESTVAR_GET(SFX_VOLUME_QVAR));
#ifdef AUDIOLOGS
    recompute_audiolog_level(QUESTVAR_GET(ALOG_VOLUME_QVAR));
    //audiolog_setting = QUESTVAR_GET(ALOG_OPT_QVAR); //moved to prefs file
#endif
    fullscrn_vitals = QUESTVAR_GET(FULLSCRN_VITAL_QVAR);
    fullscrn_icons = QUESTVAR_GET(FULLSCRN_ICON_QVAR);
    map_notes_on = QUESTVAR_GET(AMAP_NOTES_QVAR);
    hud_color_bank = QUESTVAR_GET(HUDCOLOR_QVAR);

    digichan_dealfunc(QUESTVAR_GET(DIGI_CHANNELS_QVAR));

    // mouse_set_lefty(QUESTVAR_GET(MOUSEHAND_QVAR));

    language_change(QUESTVAR_GET(LANGUAGE_QVAR));

    return (OK);
}

// char saveArray[16];	//Â¥temp

errtype load_game(char *fname) {
    int filenum;
    ObjID old_plr;
    uchar bad_save = FALSE;
    char orig_lvl;
    extern uint dynmem_mask;

    INFO("load_game %s", fname);

    empty_slate();

    closedown_game(TRUE);
    // KLC - don't do this here   stop_music();

    // Copy the save file into the current game
    copy_file(fname, CURRENT_GAME_FNAME);

    // Load in player and current level
    filenum = ResOpenFile(CURRENT_GAME_FNAME);
    old_plr = player_struct.rep;
    orig_lvl = player_struct.level;

    ResExtract(SAVE_GAME_ID_BASE + 1, FORMAT_PLAYER, (void *)&player_struct);

    obj_check_time = 0; // KLC - added because it needs to be reset for Mac version.

    // KLC - this is a global pref now.    change_detail_level(player_struct.detail_level);
    player_struct.rep = old_plr;
    player_set_eye_fixang(player_struct.eye_pos);
    if (!bad_save)
        obj_move_to(PLAYER_OBJ, &(player_struct.realspace_loc), FALSE);

    if (load_game_schedules() != OK)
        bad_save = TRUE;

    ResCloseFile(filenum);

    if (orig_lvl == player_struct.level) {
        //      Warning(("HEY, trying to be clever about loading the game! %d vs %d\n",orig_lvl,player_struct.level));
        dynmem_mask = DYNMEM_PARTIAL;
    }

    load_level_from_file(player_struct.level);
    obj_load_art(FALSE); // KLC - added here (removed from load_level_data)
    // KLC   string_message_info(REF_STR_LoadGameLoaded);
    dynmem_mask = DYNMEM_ALL;
    chg_set_flg(_current_3d_flag);
    old_ticks = *tmd_ticks;
    interpret_qvars();
    startup_game(FALSE);

    // KLC - do following instead     recompute_music_level(QUESTVAR_GET(MUSIC_VOLUME_QVAR));
    if (music_on) {
        mlimbs_on = TRUE;
        mlimbs_AI_init();
        mai_intro();                                         // KLC - added here
        load_score_for_location(PLAYER_BIN_X, PLAYER_BIN_Y); // KLC - added here
    }

    // CC: Should we go back into fullscreen mode?
    if (player_struct.hardwarez_status[CPTRIP(FULLSCR_HARD_TRIPLE)]) {
        _new_mode = FULLSCREEN_LOOP;
        chg_set_flg(GL_CHG_LOOP);
    }

    extern uchar muzzle_fire_light;
    muzzle_fire_light = FALSE;
    if (!(player_struct.hardwarez_status[CPTRIP(LANTERN_HARD_TRIPLE)] & WARE_ON))
        lamp_turnoff(TRUE, FALSE);
    else
        lamp_turnon(TRUE, FALSE);

    //Â¥Â¥ temp
    // BlockMove(0, saveArray, 16);

    return (OK);
}

errtype load_level_from_file(int level_num) {
    errtype retval;

    INFO("Loading save %i", level_num);

    retval = load_current_map(ResIdFromLevel(level_num));

    if (retval == OK) {
        player_struct.level = level_num;

        compute_shodometer_value(FALSE);

        // if this is the first time the level is loaded, compute the inital shodan security level
        if (player_struct.initial_shodan_vals[player_struct.level] == -1)
            player_struct.initial_shodan_vals[player_struct.level] = QUESTVAR_GET(SHODAN_QV);
    }

    return (retval);
}

#ifdef NOT_YET //

void check_and_update_initial(void) {
    extern Datapath savegame_dpath;
    char archive_fname[128];
    char dpath_fn[50];
    char *tmp;
    extern char real_archive_fn[20];
    if (!DatapathFind(&savegame_dpath, CURRENT_GAME_FNAME, archive_fname)) {
        tmp = getenv("CITHOME");
        if (tmp) {
            strcpy(dpath_fn, tmp);
            strcat(dpath_fn, "\\");
        } else
            dpath_fn[0] = '\0';
        strcat(dpath_fn, "data\\");
        strcat(dpath_fn, CURRENT_GAME_FNAME);

        if (!DatapathFind(&DataDirPath, real_archive_fn, archive_fname))
            critical_error(CRITERR_RES | 0x10);
        if (copy_file(archive_fname, dpath_fn) != OK)
            critical_error(CRITERR_FILE | 0x7);
    }
}

#endif // NOT_YET

uchar create_initial_game_func(short undefined1, ulong undefined2, void *undefined3) {
    int i;
    extern int actual_score;
    byte plrdiff[4];
    char tmpname[sizeof(player_struct.name)];
    short plr_obj;

    INFO("Starting game");
    DEBUG("Game archive at %s", ARCHIVE_FNAME);

    // Copy archive into local current game file.

    if (copy_file(ARCHIVE_FNAME, CURRENT_GAME_FNAME) != OK)
        critical_error(CRITERR_FILE | 7);

    plr_obj = PLAYER_OBJ;
    for (i = 0; i < 4; i++)
        plrdiff[i] = player_struct.difficulty[i];
    LG_memcpy(tmpname, player_struct.name, sizeof(tmpname));

    // KLC - don't need this anymore.  ResExtract(SAVE_GAME_ID_BASE + 1, (void *)&player_struct);

    init_player(&player_struct);
    obj_check_time = 0; // KLC - added here cause it needs to be reset in Mac version

    player_struct.rep = OBJ_NULL;

    load_level_from_file(player_struct.level);

    obj_load_art(FALSE); // KLC - added here (removed from load_level_data)
    amap_reset();

    player_create_initial();

    LG_memcpy(player_struct.name, tmpname, sizeof(player_struct.name));
    for (i = 0; i < 4; i++)
        player_struct.difficulty[i] = plrdiff[i];

    // KLC - not needed any longer ResCloseFile(filenum);

    // Reset MFDs to be consistent with starting setup
    init_newmfd();

    // No time elapsed, really, honest
    old_ticks = *tmd_ticks;

    // Setup some start-game stuff
    // Music
    current_score = actual_score = last_score = PERIL_SCORE; // KLC - these aren't actually
    mlimbs_peril = 1000;                                     // going to do anything.

    if (music_on) {
        mlimbs_on = TRUE;
        mlimbs_AI_init();
        mai_intro();                                         // KLC - added here
        load_score_for_location(PLAYER_BIN_X, PLAYER_BIN_Y); // KLC - added here
    }

    load_dynamic_memory(DYNMEM_ALL);

    // KLC - if not already on, turn on-line help on.
    if (!olh_active)
        toggle_olh_func(0, 0, 0);

    // Do entry-level triggers for starting level
    // Hmm, do we actually want to call this any time we restore
    // a saved game or whatever?  No, probably not....hmmm.....

    do_level_entry_triggers();

    // turn on help overlay.
    olh_overlay_on = olh_active;

    // Plot timers

    return (FALSE);
}

errtype write_level_to_disk(int idnum, uchar flush_mem) {
    // Eventually, this ought to cleverly determine whether or not to pack
    // the save game resource, but for now we will always do so...

    // FSMakeFSSpec(gDataVref, gDataDirID, CURRENT_GAME_FNAME, &currSpec);

    // char* currSpec = "saves/save.dat";
    return (save_current_map(CURRENT_GAME_FNAME, idnum, flush_mem, TRUE));
}
