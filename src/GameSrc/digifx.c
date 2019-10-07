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
 * $Source: r:/prj/cit/src/RCS/digifx.c $
 * $Revision: 1.48 $
 * $Author: dc $
 * $Date: 1994/11/28 08:38:42 $
 */

#include "criterr.h"
#include "objects.h"
#include "player.h"
#include "musicai.h"
#include "faketime.h"
#include "tools.h"
#include "trigger.h"
#ifdef AUDIOLOGS
#include "audiolog.h"
#endif

#define NUM_DIGI_FX 114

char volumes[NUM_DIGI_FX];
char flags[NUM_DIGI_FX];
char priorities[NUM_DIGI_FX];

// This has to be changed if the resource changes location!
#define SFX_BASE 201

extern uchar curr_alog_vol;

#ifdef NOT_YET

//#define ASYNCH_DIGI

int digi_timer_id;
void start_asynch_digi_fx() {
#ifdef ASYNCH_DIGI
    if (sfx_on)
        tm_activate_process(digi_timer_id);
#endif
}

void stop_asynch_digi_fx() {
#ifdef ASYNCH_DIGI
    if (sfx_on)
        tm_deactivate_process(digi_timer_id);
#endif
}

#endif

errtype stop_digi_fx() {
#ifdef AUDIOLOGS
    if (audiolog_setting)
        audiolog_stop();
#endif
    if (sfx_on) {
        extern void sound_frame_update(void);
        snd_kill_all_samples();
        sound_frame_update();
    }
    return (ERR_NOEFFECT);
}

void clear_digi_fx() { stop_digi_fx(); }

//#define SND_TEST

errtype digifx_init() {

    FILE *fp = fopen_caseless("res/data/digiparm.bin", "rb");
    if (fp == NULL) {
        printf("Failed to open digiparm.bin\n");
        return ERR_FOPEN;
    }

    fread(volumes, NUM_DIGI_FX, 1, fp);
    fread(flags, NUM_DIGI_FX, 1, fp);
    fread(priorities, NUM_DIGI_FX, 1, fp);
    fclose(fp);
    // clear_digi_fx();

    // snd_finish = digifx_EOS_callback;

    /* KLC - not needed now.
    #ifdef ASYNCH_DIGI
       extern void asynch_digi_fx(void);
       digi_timer_id = tm_add_process(asynch_digi_fx, 0, CIT_FREQ << 2);
    #endif
    */
    return (OK);
}

#define DIGIFX_TIMEOUT_TICKS (CIT_CYCLE * 3) >> 1
#define DIGIFX_DUPE_TICKS CIT_CYCLE >> 2

// Returns a 0-255 factor of how loud the sound should be.
#define VOL_FULL 0xFF
#define FIX_VOL_FULL (fix_make(VOL_FULL, 0))
#define MAX_DIGIFX_DIST fix_make(15, 0)
#define MIN_DIGIFX_DIST fix_make(2, 0)

extern int curr_alog;

// ------------
//  PROTOYTPES
// ------------
int compute_sfx_vol(ushort x1, ushort y1, ushort x2, ushort y2);
int compute_sfx_pan(ushort x1, ushort y1, ushort x2, ushort y2, fixang our_ang);
uchar set_sample_pan_gain(snd_digi_parms *sdp);

int compute_sfx_vol(ushort x1, ushort y1, ushort x2, ushort y2) {
    fix dx, dy, dist;
    int retval;

    dx = fix_from_obj_coord(x1) - fix_from_obj_coord(x2);
    dy = fix_from_obj_coord(y1) - fix_from_obj_coord(y2);
    dist = fix_fast_pyth_dist(dx, dy);

    if (dist > MAX_DIGIFX_DIST)
        retval = 0;
    else if (dist < MIN_DIGIFX_DIST)
        retval = VOL_FULL;
    else
        // What, no fix_mul_div_int?
        retval = fix_int(fix_mul_div(FIX_VOL_FULL, MAX_DIGIFX_DIST - dist, MAX_DIGIFX_DIST - MIN_DIGIFX_DIST));
    return (retval);
}

// i should just fix this....
int compute_sfx_pan(ushort x1, ushort y1, ushort x2, ushort y2, fixang our_ang) {
    fixang sfx_ang;
    fix dx, dy;
    int retval;

    dx = fix_from_obj_coord(x1) - fix_from_obj_coord(x2);
    dy = fix_from_obj_coord(y1) - fix_from_obj_coord(y2);

    // Do some trig, and move the angle into our relative frame
    // wait, our_ang is 0-65536 from North, clockwise  // so, ah, what is going on????
    our_ang = 0x4000 - our_ang;
    sfx_ang = fix_atan2(dy, dx) - our_ang;
    // Now we have an angle, sfx_ang, which supposedly represents the angle of the source relative to our facing...
    retval = fix_int(fix_mul(fix_fastsin(sfx_ang), fix_make(-63, 0))) +
             64; // was cos,i made it sin, flipped to be left-right
    return (retval);
}

// Returns whether or not in the humble opinion of the
// sound system, the sample should be politely obliterated out of existence
uchar set_sample_pan_gain(snd_digi_parms *sdp) {
    uchar temp_vol, vol;
    uint raw_data = (uint)sdp->data;
    extern uchar curr_sfx_vol;

    if (raw_data & 0x80000000) {
		//terrain elevator
        short x = (raw_data & 0x7FFF0000) >> 16;
        short y = (raw_data & 0xFFFF);

        temp_vol = compute_sfx_vol(x, y, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y);
        sdp->pan = compute_sfx_pan(x, y, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y, objs[PLAYER_OBJ].loc.h << 8);
    } else if (raw_data != 0) {
        ObjID id;
        id = (ObjID)raw_data;
        temp_vol = compute_sfx_vol(objs[id].loc.x, objs[id].loc.y, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y);
        sdp->pan = compute_sfx_pan(objs[id].loc.x, objs[id].loc.y, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y,
                                   objs[PLAYER_OBJ].loc.h << 8);
    } else if (sdp->snd_ref == 0) { // audiolog
                                    // following is temp      sdp->vol=curr_alog_vol;
        sdp->vol = curr_alog_vol;
        snd_sample_reload_parms(sdp);
        return (FALSE);
    } else {
        //      sdp->pan=SND_DEF_PAN;
        temp_vol = VOL_FULL;
    }
    vol = volumes[sdp->snd_ref - SFX_BASE];
    if (vol == -1)
        vol = 127;
    vol = vol * curr_sfx_vol / 100;
    sdp->vol = vol * temp_vol / VOL_FULL;
    snd_sample_reload_parms(sdp);
    return (FALSE);
}

void stop_terrain_elevator_sound(short sem)
{
  int i;

  //stop all sound channels that are playing terrain elevator sound with semaphore index sem
  for (i = 0; i < SND_MAX_SAMPLES; i++)
  {
    snd_digi_parms *sdp = snd_sample_parms(i);
    uint raw_data = (uint)sdp->data;

    if (raw_data & 0x80000000)
    {
      short x = (raw_data & 0x7FFF0000) >> 16;
      short y = (raw_data & 0xFFFF);

      extern height_semaphor h_sems[NUM_HEIGHT_SEMAPHORS];

      if (h_sems[sem].x == (x >> 8) && h_sems[sem].y == (y >> 8) && h_sems[sem].inuse)
        snd_end_sample(i);
    }
  }
}

#ifdef NOT_YET //

#pragma disable_message(202)
int digifx_volume_shift(short x, short y, short z, short phi, short theta, int basevol) {
    int retval;
    // Note that "x" is really the object ID of the thing we care about
    // unless phi is set, in which case phi and theta are a literal location to use
    if (x != OBJ_NULL)
        retval = compute_sfx_vol(objs[x].loc.x, objs[x].loc.y, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y);
    else if (phi != 0)
        retval = compute_sfx_vol(phi, theta, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y);
    else
        retval = VOL_FULL;

    // Now normalize vs basevol
    retval = basevol * retval / VOL_FULL;
    return (retval);
}

int digifx_pan_shift(short x, short y, short z, short phi, short theta) {
    int retval;
    // Note that "x" is really the object ID of the thing we care about
    // unless phi is set, in which case phi and theta are a literal location to use
    if (x != OBJ_NULL)
        retval = compute_sfx_pan(objs[x].loc.x, objs[x].loc.y, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y,
                                 objs[PLAYER_OBJ].loc.h << 8);
    else if (phi != 0)
        retval =
            compute_sfx_pan(phi, theta, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y, objs[PLAYER_OBJ].loc.h << 8);
    else
        retval = 128; // is this right??
    retval = (retval + 64) >> 1;
    Spew(DSRC_AUDIO_Testing, ("Modified PAN value=%d\n", retval));
    return (retval);
}
#pragma enable_message(202)

#endif // NOT_YET

uchar sfx_volume_levels[] = {0, 0x9, 0xF};
#define ALWAYS_QUEUE_TOLERANCE 2
#define NO_GAIN_THRESHOLD 0x6A
#define HARSH_GAIN_THRESHOLD 0xBA

snd_digi_parms s_dprm;
char secret_global_pan = SND_DEF_PAN;

int play_digi_fx_master(int sfx_code, int num_loops, ObjID id, ushort x, ushort y) {
    Id vocRes;
    int retval, real_code = sfx_code, len;
    uchar *addr;
    extern uchar sfx_on;
    extern uchar curr_sfx_vol;

    if (!sfx_on)
        return -2;
    if ((sfx_code == -1) || (sfx_code == 255))
        return -1; // why do we call this with things we dont use?

#ifdef AUDIOLOGS
    if (sfx_code > 255)
        sfx_code = 0;
    if (audiolog_playing(-1)) // what is this, really?
        if (sfx_code != real_code)
            audiolog_stop();
    if (sfx_code == real_code)
#endif
    {
        // If the sound effect is too far away, don't even bother us
        if (id != OBJ_NULL) {
            if (compute_sfx_vol(objs[id].loc.x, objs[id].loc.y, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y) == 0)
                return -1;
        } else if (x != 0)
            if (compute_sfx_vol(x, y, objs[PLAYER_OBJ].loc.x, objs[PLAYER_OBJ].loc.y) == 0)
                return -1;
    }

    vocRes = SFX_BASE + real_code;

    // s_dprm is the static data set for the parameters
    s_dprm.loops = num_loops;
    s_dprm.pri = priorities[sfx_code];
    s_dprm.pan = secret_global_pan;
#ifdef AUDIOLOGS
    if (sfx_code != real_code) {
        s_dprm.data = 0;
        s_dprm.vol = volumes[sfx_code] * curr_sfx_vol / 100;
    } else
#endif
    {
        if (id != OBJ_NULL)
            s_dprm.data = id;
        else if (x != 0)
            s_dprm.data = (0x80000000 | (x << 16) | y);
        else
            s_dprm.data = 0;
        s_dprm.snd_ref = vocRes; // okay, I'm cheating a little here
        set_sample_pan_gain(&s_dprm);
    }

    // have to hash x,y no id to a secret ID code, eh?
    s_dprm.flags = 0;
    len = ResSize(vocRes);
    addr = (uchar *)ResLock(vocRes, FORMAT_RAW);
    if (addr != NULL) {
        retval = snd_sample_play(vocRes, len, addr, &s_dprm);
    } else
        critical_error(CRITERR_MEM | 9);
    if (retval == SND_PERROR) {
        ResUnlock(vocRes);
        return -3;
    }
    return retval; // which sample id
}

// scan through the whole list
uchar digi_fx_playing(int fx_id, int *handle_ptr) {
    snd_digi_parms *sdp;

    if (fx_id == -1)
        return FALSE;

    // should scan all current sfx's for snd_ref=fx_id+SFX_BASE
    for (int i = 0; i < SND_MAX_SAMPLES; i++) {
        if (snd_sample_playing(i)) {
            sdp = snd_sample_parms(i);
            if (sdp->snd_ref == fx_id + SFX_BASE) {
                if (handle_ptr != NULL)
                    *handle_ptr = i;
                return TRUE;
            }
        }
    }
    return FALSE;
}
