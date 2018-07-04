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
 * $Source: r:/prj/cit/src/RCS/drugs.c $
 * $Revision: 1.43 $
 * $Author: mahk $
 * $Date: 1994/09/06 12:05:30 $
 *
 */

// Drugs.c    Drugs Module
// Elisha Wiesel (SPAZ)
//
// The main C file for drug effects, updates, (un)installs.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Prefs.h"
#include "cybstrng.h"
#include "drugs.h"
#include "fatigue.h"
#include "gameloop.h"
#include "gamestrn.h"
#include "init.h"
#include "mainloop.h"
#include "miscqvar.h"
#include "musicai.h"
#include "newmfd.h"
#include "objprop.h"
#include "objsim.h"
#include "objwarez.h"
#include "player.h"
#include "sfxlist.h"
#include "tools.h"

#define MAX_UBYTE 0xFF

#define INTENSITY(x) (player_struct.drug_intensity[x])
#define STATUS(x) (player_struct.drug_status[x])

// ---------
// Constants
// ---------

#define DRUG_UPDATE_FREQ 1024 // # of ticks in between updates      ~3.5 secs
#define DRUG_DECAY 8          // Std chance a drug wears off each
#define DRUG_DECAY_MAX 16     //   update is DECAY over DECAY_MAX
#define DRUG_DEFAULT_TIME \
    8 // relative "duration" of a drug
      //   duration unit is 7 * 16/8  =  14 secs

// Flags
#define DRUG_NONE 0x00        // No flags
#define DRUG_LONGER_DOSE 0x01 // If set, taking 2+ of a drug ups duration

// ----------
// Structures
// ----------

typedef struct {
    uint8_t duration; // Preset duration of given drug
    uint8_t flags;
    void (*use)(); // Function slots for take, effect, wear off
    void (*effect)();
    void (*wearoff)();
    void (*startup)(void);
    void (*closedown)(bool visible);
    void (*after_effect)();
} DRUG;

// -------
// Globals
// -------

extern DRUG Drugs[NUM_DRUGZ]; // Global array of drugs

// ----------
// Prototypes
// ----------
void drug_detox_effect();

// ===========================================================================
//    INFRASTRUCTURE
// ===========================================================================

// ---------------------------------------------------------------------------
// drug2triple()      MAHK 7/27
//
// Maps drug types (indices in to player_struct.drugs) into objtriples

int drug2triple(int type) { return MAKETRIP(CLASS_DRUG, DRUG_SUBCLASS_STATS, type); }

// ---------------------------------------------------------------------------
// triple2drug()      MAHK 7/27
//
// Maps triples onto drug types

int triple2drug(int triple) { return TRIP2TY(triple); }

// ---------------------------------------------------------------------------

// get_drug_name()
//
// Returns the stringname of drug n

char *get_drug_name(int n, char *buf) {
    int triple;

    triple = drug2triple(n);
    get_object_short_name(triple, buf, 50);

    return buf;
}

// ---------------------------------------------------------------------------
// drug_use()
//
// Use an instance of drug n, install it as appropriate.

void drug_use(int n) {
    char buf[80];
    if (player_struct.drugs[n] == 0)
        return;

    play_digi_fx(SFX_PATCH_USE, 1);
    get_drug_name(n, buf);
    get_string(REF_STR_Applied, buf + strlen(buf), sizeof(buf) - strlen(buf));
    message_info(buf);
    player_struct.drugs[n]--;

    if (Drugs[n].flags & DRUG_LONGER_DOSE)
        player_struct.drug_status[n] = lg_min((short)player_struct.drug_status[n] + Drugs[n].duration, 0x7F);
    else
        player_struct.drug_status[n] = Drugs[n].duration;

    if (Drugs[n].use)
        Drugs[n].use();

    mfd_notify_func(MFD_BIOWARE_FUNC, MFD_INFO_SLOT, FALSE, MFD_ACTIVE, FALSE);
    if (_current_loop <= FULLSCREEN_LOOP)
        chg_set_flg(INVENTORY_UPDATE);

    return;
}

// ---------------------------------------------------------------------------
// drug_wear_off()
//
// Uninstall the effect of drug n as appropriate.

void drug_wear_off(int n) {
    player_struct.drug_intensity[n] = 0;
    player_struct.drug_status[n] = 0; // in case not called from update loop

    if (Drugs[n].wearoff)
        Drugs[n].wearoff();

    mfd_notify_func(MFD_BIOWARE_FUNC, MFD_INFO_SLOT, FALSE, MFD_ACTIVE, FALSE);

    return;
}

// --------------------------------------------------------------------------
// drugs_update()
//
// This loop should get called as part of the main loop.  It keeps track of
// the last time it was called, such that it actually executes only
// once every FREQ game ticks, as defined above

void drugs_update() {
    int i, decay;

    // Only update drugs once every FREQ game ticks
    if ((player_struct.game_time - player_struct.last_drug_update) >= DRUG_UPDATE_FREQ) {

        // Reset the last update
        player_struct.last_drug_update = player_struct.game_time;

        // drugs should wear out faster if health=low or fatigue=high
        // THIS FORMULA SHOULD BE MADE INTELLIGENT.
        decay = DRUG_DECAY + (player_struct.fatigue >> 10) / 16;

        // Iterate through drugs array
        for (i = 0; i < NUM_DRUGZ; i++) {
            // If effect is active...
            if (player_struct.drug_status[i] > 0) {

                // Do something, if drug has continual effects
                if (Drugs[i].effect)
                    Drugs[i].effect();

                // Figure out if the drug wore off a little
                if ((rand() % DRUG_DECAY_MAX) < decay) {

                    // it did
                    player_struct.drug_status[i]--;

                    if (player_struct.drug_status[i] == 0) // Totally wore off?
                        drug_wear_off(i);                  // yes
                }
            } else if (player_struct.drug_status[i] < 0) // after affect
            {

                // Do something, if drug has continual effects

                // Figure out if the drug wore off a little
                if ((rand() % DRUG_DECAY_MAX) < decay) {

                    // it did
                    player_struct.drug_status[i]++;
                }
                if (Drugs[i].after_effect)
                    Drugs[i].after_effect();
            }
        }
    }

    return;
}

// ---------------------------------------------------------------------------
// drugs_init()
//
// initialize drug system.

void drugs_init() {}

//---------------------------------------------------------------------------
// drug_startup()
//
// do drug startup on game load.

void drug_startup(bool visible) {
    int i;
    for (i = 0; i < NUM_DRUGZ; i++)
        if (Drugs[i].startup != NULL)
            Drugs[i].startup();
}

void drug_closedown(bool visible) {
    int i;
    for (i = 0; i < NUM_DRUGZ; i++)
        if (Drugs[i].closedown != NULL)
            Drugs[i].closedown(visible);
}

    //---------------------------------------------------------------------------
    //

    // ===========================================================================
    //    ACTUAL DRUG FUNCTIONS
    // ===========================================================================

    // ---------------------------------------------------------------------------
    //                              DUMMY FUNCTIONS
    // ---------------------------------------------------------------------------

#ifdef USE_DUMMY_FUNCS
// ---------------------------------------------------------------------------
// dummy_use_drug()
//
// A dummy function for using drugs.

void dummy_use_drug() { return; }

// ---------------------------------------------------------------------------
// dummy_effect_drug()
//
// A dummy function for continual drug effects

void dummy_effect_drug() { return; }

// ---------------------------------------------------------------------------
// dummy_wearoff_drug()
//
// A dummy function for drugs wearing off.

void dummy_wearoff_drug() { return; }
#endif

// --------------------------------------------------------------------------
//                              LSD
// --------------------------------------------------------------------------
void drug_lsd_effect();
void drug_lsd_startup(void);
void drug_lsd_wearoff();
void drug_lsd_closedown(bool visible);

// ---------------------------------------------------------------------------
// drug_lsd_effect()
//
// Carry out a random palette swap, as an LSD effect.

void drug_lsd_effect() {
    int i;
    uchar lsd_palette[768];
    int lsd_first, lsd_last;

    // Generate criteria for a random palette shift
    lsd_first = (rand() % 253) + 1;
    lsd_last = (rand() % 253) + 1;
    if (lsd_last < lsd_first) {
        i = lsd_first;
        lsd_first = lsd_last;
        lsd_last = i;
    }
    for (i = 0; i <= ((lsd_last - lsd_first) * 3); i++)
        lsd_palette[i] = (uchar)(rand() % 256);

    gr_set_pal(lsd_first, (lsd_last - lsd_first + 1), lsd_palette);

    return;
}

void drug_lsd_startup(void) {
    if (STATUS(DRUG_LSD) > 0) {
#ifdef CRAZE_NODEATH
        // super-secret craze hack setup
        if (player_struct.hit_points == 1)
            player_struct.hit_points = 2;
#endif // CRAZE_NODEATH
        drug_lsd_effect();
    }
}

// --------------------------------------------------------------------------
// drug_lsd_wearoff()
//
// Uninstall the LSD effect.

void drug_lsd_wearoff() {
    // Return from palette shift
    gr_set_pal(0, 256, ppall);
    // KLC   gamma_dealfunc(QUESTVAR_GET(GAMMACOR_QVAR));
    gamma_dealfunc(gShockPrefs.doGamma);

    return;
}

void drug_lsd_closedown(bool visible) {
    if (visible && STATUS(DRUG_LSD) > 0)
        gr_set_pal(0, 256, ppall);
}

// --------------------------------------------------------------------------
//                              STAMINUP
// --------------------------------------------------------------------------
void drug_staminup_use();
void drug_staminup_effect();
void drug_staminup_wearoff();

// ---------------------------------------------------------------------------
// drug_staminup_use()
//
// Initial effects of the staminup drug.

extern uchar fatigue_warning;

void drug_staminup_use() {
    player_struct.fatigue = 0;
    return;
}

// ---------------------------------------------------------------------------
// drug_staminup_effect()
//
// Continual effects of the staminup drug.

void drug_staminup_effect() {
    player_struct.fatigue = 0;
    return;
}

// ---------------------------------------------------------------------------
// drug_staminup_wearoff()
//
// Final effects of the staminup drug.

void drug_staminup_wearoff() {
    player_struct.fatigue = MAX_FATIGUE;
    return;
}

// --------------------------------------------------------------------------
//                              SIGHT
// --------------------------------------------------------------------------
void drug_sight_use();
void drug_sight_startup();
void drug_sight_effect();
void drug_sight_wearoff();
void drug_sight_after_effect(void);
void drug_sight_closedown(bool visible);

extern void set_global_lighting(short);
#define SIGHT_LIGHT_LEVEL (4 << 8)

// ---------------------------------------------------------------------------
// drug_sight_use()
//
// Initial effects of the sight drug.

void drug_sight_use() {
    if (INTENSITY(DRUG_SIGHT) == 0)
        set_global_lighting(SIGHT_LIGHT_LEVEL);
    INTENSITY(DRUG_SIGHT) = 1;
    return;
}

void drug_sight_startup() {
    if (STATUS(DRUG_SIGHT) > 0) {
        set_global_lighting(SIGHT_LIGHT_LEVEL);
    } else if (STATUS(DRUG_SIGHT) < 0) {
        set_global_lighting(-SIGHT_LIGHT_LEVEL);
    }
    return;
}

// ---------------------------------------------------------------------------
// drug_sight_effect()
//
// Continual effects of the sight drug.

void drug_sight_effect() { return; }

// ---------------------------------------------------------------------------
// drug_sight_wearoff()
//
// Final effects of the sight drug.

void drug_sight_wearoff() {
    INTENSITY(DRUG_SIGHT) = 0;
    set_global_lighting(-2 * SIGHT_LIGHT_LEVEL);
    STATUS(DRUG_SIGHT) = -Drugs[DRUG_SIGHT].duration / 4;
    return;
}

void drug_sight_after_effect(void) {
    if (STATUS(DRUG_SIGHT) == 0)
        set_global_lighting(SIGHT_LIGHT_LEVEL);
}

void drug_sight_closedown(bool visible) {
    if (!visible)
        return;
    if (STATUS(DRUG_SIGHT) > 0)
        set_global_lighting(-SIGHT_LIGHT_LEVEL);
    else if (STATUS(DRUG_SIGHT) < 0)
        set_global_lighting(SIGHT_LIGHT_LEVEL);
    return;
}

// --------------------------------------------------------------------------
//                              MEDIC
// --------------------------------------------------------------------------
void drug_medic_use();
void drug_medic_effect();
void drug_medic_wearoff();

#define MEDIC_DECAY_RATE 8
#define MEDIC_HEAL_STEPS 10
// ushort medic_heal_rates[] = { 10,50,55,105,210} ;
ushort medic_heal_rates[] = {5, 5, 25, 25, 25, 30, 50, 55, 100, 110};
#define MEDIC_HEAL_RATE 430

// ---------------------------------------------------------------------------
// drug_medic_use()
//
// Initial effects of the medic drug.

void drug_medic_use() {
    INTENSITY(DRUG_MEDIC) = lg_min(INTENSITY(DRUG_MEDIC) + MEDIC_HEAL_STEPS, MAX_UBYTE);
    player_struct.hit_points_regen += MEDIC_HEAL_RATE;
    chg_set_flg(VITALS_UPDATE);

    return;
}

// ---------------------------------------------------------------------------
// drug_medic_effect()
//
// Continual effects of the medic drug.

void drug_medic_effect() {
    ubyte n = INTENSITY(DRUG_MEDIC);
    if (n > 0) {
        short delta;
        if (n > MEDIC_HEAL_STEPS) {
            delta = MEDIC_HEAL_RATE;
            n -= MEDIC_HEAL_STEPS;
        } else {
            n--;
            delta = medic_heal_rates[n % MEDIC_HEAL_STEPS];
        }
        player_struct.hit_points_regen -= delta;
    }
    INTENSITY(DRUG_MEDIC) = n;
    if (n == 0)
        STATUS(DRUG_MEDIC) = 0;
    return;
}

// ---------------------------------------------------------------------------
// drug_medic_wearoff()
//
// Final effects of the medic drug.

void drug_medic_wearoff() {
    while (INTENSITY(DRUG_MEDIC) > 0)
        drug_medic_effect();

    INTENSITY(DRUG_MEDIC) = 0;
    return;
}

// --------------------------------------------------------------------------
//                              REFLEX
// --------------------------------------------------------------------------
void drug_reflex_use();
void drug_reflex_effect();
void drug_reflex_wearoff();

// ---------------------------------------------------------------------------
// drug_reflex_use()
//
// Initial effects of the reflex drug.

void drug_reflex_use() {
    extern char reflex_remainder;
    reflex_remainder = 0;
    return;
}

// ---------------------------------------------------------------------------
// drug_reflex_effect()
//
// Continual effects of the reflex drug.

void drug_reflex_effect() { return; }

// ---------------------------------------------------------------------------
// drug_reflex_wearoff()
//
// Final effects of the reflex drug.

void drug_reflex_wearoff() { return; }

// --------------------------------------------------------------------------
//                              GENIUS
// --------------------------------------------------------------------------
void drug_genius_use();
void drug_genius_effect();
void drug_genius_wearoff();

// ---------------------------------------------------------------------------
// drug_genius_use()
//
// Initial effects of the genius drug.

void drug_genius_use() {
    void mfd_gridpanel_set_winmove(uchar check);

    mfd_gridpanel_set_winmove(true);
    return;
}

// ---------------------------------------------------------------------------
// drug_genius_effect()
//
// Continual effects of the genius drug.

void drug_genius_effect() { return; }

// ---------------------------------------------------------------------------
// drug_genius_wearoff()
//
// Final effects of the genius drug.

void drug_genius_wearoff() { return; }

// --------------------------------------------------------------------------
//                              DETOX
// --------------------------------------------------------------------------
void drug_detox_use();
void wear_off_drug(int i);
void drug_detox_wearoff();

uchar detox_drug_order[] = {
    DRUG_LSD, DRUG_SIGHT, DRUG_GENIUS, DRUG_STAMINUP, DRUG_REFLEX, DRUG_MEDIC,
};

#define NUM_DETOX_DRUGS (sizeof(detox_drug_order) / sizeof(detox_drug_order[0]))

// ---------------------------------------------------------------------------
// drug_detox_use()
//
// Initial effects of the detox drug.

void drug_detox_use() {
    INTENSITY(DRUG_DETOX) += 2;
    if (INTENSITY(DRUG_DETOX) == 2)
        drug_detox_effect();
    return;
}

// ---------------------------------------------------------------------------
// drug_detox_effect()
//
// Continual effects of the detox drug.

void wear_off_drug(int i) {
    int laststat = STATUS(i);
    STATUS(i) = 0;
    if (laststat > 0 && Drugs[i].wearoff != NULL)
        Drugs[i].wearoff();
    else if (laststat < 0 && Drugs[i].after_effect != NULL)
        Drugs[i].after_effect();
}

void drug_detox_effect() {
    int i, stack;
    for (stack = 0; stack < INTENSITY(DRUG_DETOX); stack += 2) {
        for (i = 0; i < NUM_DETOX_DRUGS; i++) {
            int d = detox_drug_order[i];
            if (STATUS(d) < 0) {
                wear_off_drug(d);
                goto found;
            }
        }
        for (i = 0; i < NUM_DETOX_DRUGS; i++) {
            int d = detox_drug_order[i];
            if (STATUS(d) > 0) {
                wear_off_drug(d);
                goto found;
            }
        }
    found:;
    }
    for (i = 0; i < NUM_DAMAGE_TYPES; i++) {
        player_struct.hit_points_lost[i] /= INTENSITY(DRUG_DETOX) + 1;
    }
}

// ---------------------------------------------------------------------------
// drug_detox_wearoff()
//
// Final effects of the detox drug.

void drug_detox_wearoff() {
    INTENSITY(DRUG_DETOX) = 0;
    return;
}

// ---------------------------------------------------------------------------

DRUG Drugs[NUM_DRUGZ] = {
    // Staminup
    {
        10,                    // duration
        DRUG_LONGER_DOSE,      // flags
        &drug_staminup_use,    // use
        &drug_staminup_effect, // effect
        &drug_staminup_wearoff // wearoff
    },
    // Sight
    {
        20,                       // duration
        DRUG_LONGER_DOSE,         // flags
        &drug_sight_use,          // use
        &drug_sight_effect,       // effect
        &drug_sight_wearoff,      // wearoff
        &drug_sight_startup,      // startup
        &drug_sight_closedown,    // closedown
        &drug_sight_after_effect, // aftereffect
    },
    // Berserk
    {
        DRUG_DEFAULT_TIME / 2, // duration
        DRUG_LONGER_DOSE,      // flags
        NULL,                  // use
        &drug_lsd_effect,      // effect
        &drug_lsd_wearoff,     // wearoff
        &drug_lsd_startup,     // startup
        &drug_lsd_closedown    // closedown
    },
    // Medic
    {
        10,                 // duration
        DRUG_NONE,          // flags
        &drug_medic_use,    // use
        &drug_medic_effect, // effect
        &drug_medic_wearoff // wearoff
    },
    // Reflex
    {
        DRUG_DEFAULT_TIME,   // duration
        DRUG_LONGER_DOSE,    // flags
        &drug_reflex_use,    // use
        &drug_reflex_effect, // effect
        &drug_reflex_wearoff // wearoff
    },

    // Genius
    {
        DRUG_DEFAULT_TIME / 2, // duration
        DRUG_NONE,             // flags
        &drug_genius_use,      // use
        &drug_genius_effect,   // effect
        &drug_genius_wearoff   // wearoff
    },
    // Detox
    {
        DRUG_DEFAULT_TIME,   // duration
        DRUG_NONE,           // flags
        &drug_detox_use,     // use
        &drug_detox_effect,  // effect
        &drug_detox_wearoff, // wearoff
    },
};
