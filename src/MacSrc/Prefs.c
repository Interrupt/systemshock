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
//====================================================================================
//
//		System Shock - ©1994-1995 Looking Glass Technologies, Inc.
//
//		Prefs.c	-	Handles saving and loading preferences.
//
//====================================================================================

//--------------------
//  Includes
//--------------------
#include "Shock.h"
#include "Prefs.h"

#include "popups.h"
#include "olhext.h"

static void SetShockGlobals(void);

//--------------------
//  Globals
//--------------------
ShockPrefs gShockPrefs;
char which_lang;
uchar sfx_on = TRUE;

//--------------------
//  Externs
//--------------------
extern int _fr_global_detail;
extern Boolean DoubleSize;
extern Boolean SkipLines;
extern short mode_id;

extern uchar curr_vol_lev;
extern uchar curr_sfx_vol;
extern uchar curr_alog_vol;

static const char *PREFS_FILENAME = "prefs.txt";

static const char *PREF_LANGUAGE     = "language";
static const char *PREF_MUSIC_VOL    = "music-volume";
static const char *PREF_SFX_VOL      = "sfx-volume";
static const char *PREF_ALOG_VOL     = "alog-volume";
static const char *PREF_VIDEOMODE    = "video-mode";
static const char *PREF_HALFRES      = "half-resoultion";
static const char *PREF_DETAIL       = "detail";

//--------------------------------------------------------------------
//	  Initialize the preferences to their default settings.
//--------------------------------------------------------------------
void SetDefaultPrefs(void) {
    gShockPrefs.prefVer = 0;
    gShockPrefs.prefPlayIntro = 1;      // First time through, play the intro

    gShockPrefs.goMsgLength = 0;        // Normal
    gShockPrefs.goPopupLabels = true;
    gShockPrefs.goOnScreenHelp = true;
    gShockPrefs.goLanguage = 0;         // English

    gShockPrefs.soBackMusic = true;
    gShockPrefs.soSoundFX = true;
    gShockPrefs.soMusicVolume = 100;
    gShockPrefs.soSfxVolume = 100;
    gShockPrefs.soAudioLogVolume = 100;

    gShockPrefs.doVideoMode = 3;
    gShockPrefs.doResolution = 0;       // High-res.
    gShockPrefs.doDetail = 3;           // Max detail.
    gShockPrefs.doGamma = 29;           // Default gamma (29 out of 100).
    gShockPrefs.doUseQD = false;

    SetShockGlobals();
}

static FILE *open_prefs(const char *mode) {
    char fullname[512];
    char *path = SDL_GetPrefPath("Interrupt", "SystemShock");
    snprintf(fullname, sizeof(fullname), "%s%s", path, PREFS_FILENAME);
    free(path);
    return fopen(fullname, mode);
}

static char *trim(char *s) {
    while (*s && isspace(*s))
        s++;
    char *c = &s[strlen(s) - 1];
    while (c >= s && isspace(*c))
        *(c--) = '\0';
    return s;
}

static bool is_true(const char *s) {
    return strcasecmp(s, "yes") == 0 || strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0;
}

//--------------------------------------------------------------------
//	  Locate the preferences file and load them to set our global pref settings.
//--------------------------------------------------------------------
OSErr LoadPrefs(void) {
    FILE *f = open_prefs("r");
    if (!f) {
        // file can't be open, write default preferences
        return SavePrefs();
    }

    char line[64];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq)
            continue;

        *eq = '\0';
        const char *key = trim(line);
        const char *value = trim(eq + 1);

        if (strcasecmp(key, PREF_LANGUAGE) == 0) {
            int lang = atoi(value);
            if (lang >= 0 && lang <= 2)
                gShockPrefs.goLanguage = lang;
        } else if (strcasecmp(key, PREF_MUSIC_VOL) == 0) {
            int vol = atoi(value);
            if (vol >= 0 && vol <= 100) {
                gShockPrefs.soBackMusic = vol > 0;
                gShockPrefs.soMusicVolume = vol;
            }
        } else if (strcasecmp(key, PREF_SFX_VOL) == 0) {
            int vol = atoi(value);
            if (vol >= 0 && vol <= 100) {
                gShockPrefs.soSoundFX = vol > 0;
                gShockPrefs.soSfxVolume = vol;
            }
        } else if (strcasecmp(key, PREF_ALOG_VOL) == 0) {
            int vol = atoi(value);
            if (vol >= 0 && vol <= 100)
                gShockPrefs.soAudioLogVolume = vol;
        } else if (strcasecmp(key, PREF_VIDEOMODE) == 0) {
            int mode = atoi(value);
            if (mode >= 0 && mode <= 4)
                gShockPrefs.doVideoMode = mode;
        } else if (strcasecmp(key, PREF_HALFRES) == 0) {
            gShockPrefs.doResolution = is_true(value);
        } else if (strcasecmp(key, PREF_DETAIL) == 0) {
            int detail = atoi(value);
            if (detail >= 0 && detail <= 3)
                gShockPrefs.doDetail = detail;
        }
    }

    fclose(f);
    SetShockGlobals();
    return 0;
}

//--------------------------------------------------------------------
//	  Save global settings in the preferences file.
//--------------------------------------------------------------------
OSErr SavePrefs(void) {
    INFO("Saving preferences");
    FILE *f = open_prefs("w");
    if (!f) {
        printf("ERROR: Failed to open preferences file\n");
        return -1;
    }

    fprintf(f, "%s = %d\n", PREF_LANGUAGE, which_lang);
    fprintf(f, "%s = %d\n", PREF_MUSIC_VOL, curr_vol_lev);
    fprintf(f, "%s = %d\n", PREF_SFX_VOL, sfx_on ? curr_sfx_vol : 0);
    fprintf(f, "%s = %d\n", PREF_ALOG_VOL, curr_alog_vol);
    fprintf(f, "%s = %d\n", PREF_VIDEOMODE, mode_id);
    fprintf(f, "%s = %s\n", PREF_HALFRES, DoubleSize ? "yes" : "no");
    fprintf(f, "%s = %d\n", PREF_DETAIL, _fr_global_detail);
    fclose(f);

    return 0;
}

//--------------------------------------------------------------------
//  Set the corresponding Shock globals from the prefs structure.
//--------------------------------------------------------------------
static void SetShockGlobals(void) {
    popup_cursors = gShockPrefs.goPopupLabels;
    olh_active = gShockPrefs.goOnScreenHelp;
    which_lang = gShockPrefs.goLanguage;

    sfx_on = gShockPrefs.soSoundFX;
    curr_vol_lev = gShockPrefs.soMusicVolume;
    curr_sfx_vol = gShockPrefs.soSfxVolume;
    curr_alog_vol = gShockPrefs.soAudioLogVolume;

    mode_id = gShockPrefs.doVideoMode;
    DoubleSize = (gShockPrefs.doResolution == 1); // Set this True for low-res.
    SkipLines = gShockPrefs.doUseQD;
    _fr_global_detail = gShockPrefs.doDetail;
}
