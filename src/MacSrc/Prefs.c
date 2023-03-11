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
//                  Also loads and sets default keybinds.
//
//====================================================================================

//--------------------
//  Includes
//--------------------
#include "Shock.h"
#include "Prefs.h"

#include "popups.h"
#include "olhext.h"
#include "hotkey.h"
#include "input.h"
#include "mainloop.h"
#include "movekeys.h"
#include "mfdext.h"

extern uchar mfd_button_callback_kb(ushort keycode, uint32_t context, intptr_t data);
extern uchar hw_hotkey_callback(ushort keycode, uint32_t context, intptr_t data);

//--------------------
//  Filenames
//--------------------
static const char *PREFS_FILENAME = "prefs.txt";
static const char *KEYBINDS_FILENAME = "keybinds.txt";

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
extern bool DoubleSize;
extern bool SkipLines;
extern short mode_id;

extern uchar curr_vol_lev;
extern uchar curr_sfx_vol;
extern uchar curr_alog_vol;

extern uchar audiolog_setting;

static const char *PREF_LANGUAGE = "language";
static const char *PREF_CAPTUREMOUSE = "capture-mouse";
static const char *PREF_INVERTMOUSEY = "invert-mousey";
static const char *PREF_MUSIC_VOL = "music-volume";
static const char *PREF_SFX_VOL = "sfx-volume";
static const char *PREF_ALOG_VOL = "alog-volume";
static const char *PREF_VIDEOMODE = "video-mode";
static const char *PREF_HALFRES = "half-resolution";
static const char *PREF_DETAIL = "detail";
static const char *PREF_USE_OPENGL = "use-opengl";
static const char *PREF_TEX_FILTER = "texture-filter";
static const char *PREF_ONSCR_HELP = "onscreen-help";
static const char *PREF_GAMMA = "gamma";
static const char *PREF_MSG_LENGTH = "message-length";
static const char *PREF_ALOG_SETTING = "alog-setting";
static const char *PREF_MIDI_BACKEND = "midi-backend";
static const char *PREF_MIDI_OUTPUT = "midi-output";

static void SetShockGlobals(void);

//--------------------------------------------------------------------
//	  Initialize the preferences to their default settings.
//--------------------------------------------------------------------
void SetDefaultPrefs(void) {

    gShockPrefs.prefVer = 0;
    gShockPrefs.prefPlayIntro = 1; // First time through, play the intro
    gShockPrefs.goPopupLabels = true;
    gShockPrefs.soBackMusic = true;
#ifdef USE_FLUIDSYNTH
    gShockPrefs.soMidiBackend = 2; // default to fluidsynth when available
#else
    gShockPrefs.soMidiBackend = 0; // default to adlmidi
#endif
    gShockPrefs.soMidiOutput = 0; // default to zero
    gShockPrefs.soSoundFX = true;
    gShockPrefs.doUseQD = false;

    // saved in prefs file

    gShockPrefs.goLanguage = 0; // English
    gShockPrefs.goCaptureMouse = true;
    gShockPrefs.goInvertMouseY = false;
    gShockPrefs.soMusicVolume = 75;
    gShockPrefs.soSfxVolume = 100;
    gShockPrefs.soAudioLogVolume = 100;
    gShockPrefs.doVideoMode = 3;
    gShockPrefs.doResolution = 0; // High-res.
    gShockPrefs.doDetail = 3;     // Max detail.
    gShockPrefs.doUseOpenGL = false;
    gShockPrefs.doTextureFilter = 0; // unfiltered
    gShockPrefs.goOnScreenHelp = true;
    gShockPrefs.doGamma = 29;    // Default gamma (29 out of 100).
    gShockPrefs.goMsgLength = 0; // Normal
    audiolog_setting = 1;

    SetShockGlobals();
}

static char *GetPrefsPathFilename(void) {
    static char filename[512];

    FILE *f = fopen(PREFS_FILENAME, "r");
    if (f != NULL) {
        fclose(f);
        strcpy(filename, PREFS_FILENAME);
    } else {
        char *p = SDL_GetPrefPath("Interrupt", "SystemShock");
        snprintf(filename, sizeof(filename), "%s%s", p, PREFS_FILENAME);
        SDL_free(p);
    }

    return filename;
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
int16_t LoadPrefs(void) {
    FILE *f = fopen(GetPrefsPathFilename(), "r");
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
        } else if (strcasecmp(key, PREF_CAPTUREMOUSE) == 0) {
            gShockPrefs.goCaptureMouse = is_true(value);
        } else if (strcasecmp(key, PREF_INVERTMOUSEY) == 0) {
            gShockPrefs.goInvertMouseY = is_true(value);
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
        } else if (strcasecmp(key, PREF_USE_OPENGL) == 0) {
            gShockPrefs.doUseOpenGL = is_true(value);
        } else if (strcasecmp(key, PREF_TEX_FILTER) == 0) {
            int mode = atoi(value);
            if (mode >= 0 && mode <= 1)
                gShockPrefs.doTextureFilter = (short)mode;
        } else if (strcasecmp(key, PREF_ONSCR_HELP) == 0) {
            gShockPrefs.goOnScreenHelp = is_true(value);
        } else if (strcasecmp(key, PREF_GAMMA) == 0) {
            int gamma = atoi(value);
            if (gamma < 10)
                gamma = 10;
            if (gamma > 100)
                gamma = 100;
            gShockPrefs.doGamma = gamma;
        } else if (strcasecmp(key, PREF_MSG_LENGTH) == 0) {
            int ml = atoi(value);
            if (ml >= 0 && ml <= 1)
                gShockPrefs.goMsgLength = ml;
        } else if (strcasecmp(key, PREF_ALOG_SETTING) == 0) {
            int as = atoi(value);
            if (as >= 0 && as <= 2)
                audiolog_setting = as;
        } else if (strcasecmp(key, PREF_MIDI_BACKEND) == 0) {
            int mb = atoi(value);
            if (mb >= 0 && mb <= 2)
                gShockPrefs.soMidiBackend = (short)mb;
        } else if (strcasecmp(key, PREF_MIDI_OUTPUT) == 0) {
            int mo = atoi(value);
            if (mo >= 0)
                gShockPrefs.soMidiOutput = (short)mo;
        }
    }

    fclose(f);
    SetShockGlobals();
    return 0;
}

//--------------------------------------------------------------------
//	  Save global settings in the preferences file.
//--------------------------------------------------------------------
int16_t SavePrefs(void) {
    INFO("Saving preferences");

    FILE *f = fopen(GetPrefsPathFilename(), "w");
    if (!f) {
        printf("ERROR: Failed to open preferences file\n");
        return -1;
    }

    fprintf(f, "%s = %d\n", PREF_LANGUAGE, which_lang);
    fprintf(f, "%s = %s\n", PREF_CAPTUREMOUSE, gShockPrefs.goCaptureMouse ? "yes" : "no");
    fprintf(f, "%s = %s\n", PREF_INVERTMOUSEY, gShockPrefs.goInvertMouseY ? "yes" : "no");
    fprintf(f, "%s = %d\n", PREF_MUSIC_VOL, curr_vol_lev);
    fprintf(f, "%s = %d\n", PREF_SFX_VOL, sfx_on ? curr_sfx_vol : 0);
    fprintf(f, "%s = %d\n", PREF_ALOG_VOL, curr_alog_vol);
    fprintf(f, "%s = %d\n", PREF_VIDEOMODE, mode_id);
    fprintf(f, "%s = %s\n", PREF_HALFRES, DoubleSize ? "yes" : "no");
    fprintf(f, "%s = %d\n", PREF_DETAIL, _fr_global_detail);
    fprintf(f, "%s = %s\n", PREF_USE_OPENGL, gShockPrefs.doUseOpenGL ? "yes" : "no");
    fprintf(f, "%s = %d\n", PREF_TEX_FILTER, gShockPrefs.doTextureFilter);
    fprintf(f, "%s = %s\n", PREF_ONSCR_HELP, gShockPrefs.goOnScreenHelp ? "yes" : "no");
    fprintf(f, "%s = %d\n", PREF_GAMMA, gShockPrefs.doGamma);
    fprintf(f, "%s = %d\n", PREF_MSG_LENGTH, gShockPrefs.goMsgLength);
    fprintf(f, "%s = %d\n", PREF_ALOG_SETTING, audiolog_setting);
    fprintf(f, "%s = %d\n", PREF_MIDI_BACKEND, gShockPrefs.soMidiBackend);
    fprintf(f, "%s = %d\n", PREF_MIDI_OUTPUT, gShockPrefs.soMidiOutput);
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

//************************************************************************************

//********
// Keybinds
//********

// Note that Alt / Option (on Mac) modifier key won't work until it is implemented in sdl_events.c

static struct {
    const char *s;
    int ch, code;
} KeyName2ChCode[] = {{"backspace ", 8, 0x33},
                      {"tab ", 9, 0x30},
                      {"enter ", 13, 0x24},
                      {"escape ", 27, 0x35},
                      {"space ", 32, 0x31},
                      {"1 ", 49, 0x12},
                      {"exclamation ", 33, 0x12},
                      {"2 ", 50, 0x13},
                      {"atsign ", 64, 0x13},
                      {"3 ", 51, 0x14},
                      {"numbersign ", 35, 0x14},
                      {"4 ", 52, 0x15},
                      {"dollar ", 36, 0x15},
                      {"5 ", 53, 0x17},
                      {"percent ", 37, 0x17},
                      {"6 ", 54, 0x16},
                      {"caret ", 94, 0x16},
                      {"7 ", 55, 0x1A},
                      {"ampersand ", 38, 0x1A},
                      {"8 ", 56, 0x1C},
                      {"asterisk ", 42, 0x1C},
                      {"9 ", 57, 0x19},
                      {"lparenthesis ", 40, 0x19},
                      {"0 ", 48, 0x1D},
                      {"rparenthesis ", 41, 0x1D},
                      {"equals ", 61, 0x18},
                      {"plus ", 43, 0x18},
                      {"comma ", 44, 0x2B},
                      {"lessthan ", 60, 0x2B},
                      {"minus ", 45, 0x1B},
                      {"underscore ", 95, 0x1B},
                      {"period ", 46, 0x2F},
                      {"greaterthan ", 62, 0x2F},
                      {"slash ", 47, 0x2C},
                      {"questionmark ", 63, 0x2C},
                      {"quote ", 39, 0x27},
                      {"doublequote ", 34, 0x27},
                      {"semicolon ", 59, 0x29},
                      {"colon ", 58, 0x29},
                      {"a ", 97, 0x00},
                      {"b ", 98, 0x0B},
                      {"c ", 99, 0x08},
                      {"d ", 100, 0x02},
                      {"e ", 101, 0x0E},
                      {"f ", 102, 0x03},
                      {"g ", 103, 0x05},
                      {"h ", 104, 0x04},
                      {"i ", 105, 0x22},
                      {"j ", 106, 0x26},
                      {"k ", 107, 0x28},
                      {"l ", 108, 0x25},
                      {"m ", 109, 0x2E},
                      {"n ", 110, 0x2D},
                      {"o ", 111, 0x1F},
                      {"p ", 112, 0x23},
                      {"q ", 113, 0x0C},
                      {"r ", 114, 0x0F},
                      {"s ", 115, 0x01},
                      {"t ", 116, 0x11},
                      {"u ", 117, 0x20},
                      {"v ", 118, 0x09},
                      {"w ", 119, 0x0D},
                      {"x ", 120, 0x07},
                      {"y ", 121, 0x10},
                      {"z ", 122, 0x06},
                      {"A ", 65, 0x00},
                      {"B ", 66, 0x0B},
                      {"C ", 67, 0x08},
                      {"D ", 68, 0x02},
                      {"E ", 69, 0x0E},
                      {"F ", 70, 0x03},
                      {"G ", 71, 0x05},
                      {"H ", 72, 0x04},
                      {"I ", 73, 0x22},
                      {"J ", 74, 0x26},
                      {"K ", 75, 0x28},
                      {"L ", 76, 0x25},
                      {"M ", 77, 0x2E},
                      {"N ", 78, 0x2D},
                      {"O ", 79, 0x1F},
                      {"P ", 80, 0x23},
                      {"Q ", 81, 0x0C},
                      {"R ", 82, 0x0F},
                      {"S ", 83, 0x01},
                      {"T ", 84, 0x11},
                      {"U ", 85, 0x20},
                      {"V ", 86, 0x09},
                      {"W ", 87, 0x0D},
                      {"X ", 88, 0x07},
                      {"Y ", 89, 0x10},
                      {"Z ", 90, 0x06},
                      {"lbracket ", 91, 0x21},
                      {"lcurbrace ", 123, 0x21},
                      {"backslash ", 92, 0x2A},
                      {"vertline ", 124, 0x2A},
                      {"rbracket ", 93, 0x1E},
                      {"rcurbrace ", 125, 0x1E},
                      {"backquote ", 96, 0x32},
                      {"tilde ", 126, 0x32},
                      {"delete ", 127, 0x33},

                      // use these invented "ascii" codes for hotkey system
                      // see sdl_events.c
                      {"f1 ", 128 + 0, 0x7A},
                      {"f2 ", 128 + 1, 0x78},
                      {"f3 ", 128 + 2, 0x63},
                      {"f4 ", 128 + 3, 0x76},
                      {"f5 ", 128 + 4, 0x60},
                      {"f6 ", 128 + 5, 0x61},
                      {"f7 ", 128 + 6, 0x62},
                      {"f8 ", 128 + 7, 0x64},
                      {"f9 ", 128 + 8, 0x65},
                      {"f10 ", 128 + 9, 0x6D},
                      {"f11 ", 128 + 10, 0x67},
                      {"f12 ", 128 + 11, 0x6F},
                      {"keypad_divide ", 128 + 12, 0x4B},
                      {"keypad_multiply ", 128 + 13, 0x43},
                      {"keypad_minus ", 128 + 14, 0x4E},
                      {"keypad_plus ", 128 + 15, 0x45},
                      {"keypad_enter ", 128 + 16, 0x4C},
                      {"keypad_decimal ", 128 + 17, 0x41},
                      {"keypad_0 ", 128 + 18, 0x52},

                      // these have no invented "ascii" codes so they can't be used as hotkeys, only move keys
                      {"keypad_home ", 0, 0x59},
                      {"keypad_up ", 0, 0x5B},
                      {"keypad_pgup ", 0, 0x5C},
                      {"keypad_left ", 0, 0x56},
                      {"keypad_5 ", 0, 0x57},
                      {"keypad_right ", 0, 0x58},
                      {"keypad_end ", 0, 0x53},
                      {"keypad_down ", 0, 0x54},
                      {"keypad_pgdn ", 0, 0x55},
                      {"home ", 0, 0x73},
                      {"up ", 0, 0x7E},
                      {"pageup ", 0, 0x74},
                      {"left ", 0, 0x7B},
                      {"right ", 0, 0x7C},
                      {"end ", 0, 0x77},
                      {"down ", 0, 0x7D},
                      {"pagedown ", 0, 0x79},

                      {NULL, 0, 0}};

// lower cases all characters in string p
// also converts tabs to spaces
static void LowerCaseInPlace(char *p) {
    while (*p) {
        if (*p >= 'A' && *p <= 'Z')
            *p = *p - 'A' + 'a'; // convert upper to lower case
        if (*p == '\t')
            *p = ' '; // convert tab to space
        p++;
    }
}

//*********************************
// Set hotkey keybinds (see input.c)
// Also handles fire keybinds
//*********************************

#ifdef AUDIOLOGS
extern uchar audiolog_cancel_func(ushort keycode, uint32_t context, intptr_t data);
#endif
extern uchar posture_hotkey_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar toggle_mouse_look(ushort keycode, uint32_t context, intptr_t data);
extern uchar change_mode_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar clear_fullscreen_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar saveload_hotkey_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar pause_game_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar reload_weapon_hotkey(ushort keycode, uint32_t context, intptr_t data);
extern uchar select_grenade_hotkey(ushort keycode, uint32_t context, intptr_t data);
extern uchar toggle_olh_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar select_drug_hotkey(ushort keycode, uint32_t context, intptr_t data);
extern uchar toggle_music_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar demo_quit_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar cycle_weapons_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar MacDetailFunc(ushort keycode, uint32_t context, intptr_t data);
extern uchar toggle_opengl_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar arm_grenade_hotkey(ushort keycode, uint32_t context, intptr_t data);
extern uchar use_drug_hotkey(ushort keycode, uint32_t context, intptr_t data);
extern uchar hud_color_bank_cycle(ushort keycode, uint32_t context, intptr_t data);
extern uchar olh_overlay_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar keypad_hotkey_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar MacHelpFunc(ushort keycode, uint32_t context, intptr_t data);
extern uchar wrapper_options_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar toggle_giveall_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar toggle_physics_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar toggle_up_level_func(ushort keycode, uint32_t context, intptr_t data);
extern uchar toggle_down_level_func(ushort keycode, uint32_t context, intptr_t data);

#define TAB_KEY (KEY_TAB | KB_FLAG_DOWN)
#define S_TAB_KEY (KEY_TAB | KB_FLAG_DOWN | KB_FLAG_SHIFT)

#define DOWN(x) ((x) | KB_FLAG_DOWN)
#define SHIFT(x) (DOWN(x) | KB_FLAG_SHIFT)
#define CTRL(x) (DOWN(x) | KB_FLAG_CTRL)
#define ALT(x) (DOWN(x) | KB_FLAG_ALT)

typedef struct HOTKEYLOOKUP_STRUCT {
    const char *s;
    intptr_t contexts;
    hotkey_callback func;
    intptr_t state;
    bool used;
    int def1, def2;
} HOTKEYLOOKUP;

HOTKEYLOOKUP HotKeyLookup[] = {
//  name                    contexts      func                    state           used  default key 1,2
#ifdef AUDIOLOGS
    {"\"audiolog_cancel\"", DEMO_CONTEXT, audiolog_cancel_func, 0, 0, CTRL('.'), 0},
#endif
    {"\"stand\"", DEMO_CONTEXT, posture_hotkey_func, 0, 0, DOWN('t'), SHIFT('t')},
    {"\"crouch\"", DEMO_CONTEXT, posture_hotkey_func, 1, 0, DOWN('g'), SHIFT('g')},
    {"\"prone\"", DEMO_CONTEXT, posture_hotkey_func, 2, 0, DOWN('b'), SHIFT('b')},
    {"\"toggle_freelook\"", DEMO_CONTEXT, toggle_mouse_look, TRUE, 0, DOWN('f'), 0},
    {"\"full_view\"", DEMO_CONTEXT, change_mode_func, FULLSCREEN_LOOP, 0, CTRL('f'), 0},
    {"\"normal_view\"", DEMO_CONTEXT, change_mode_func, GAME_LOOP, 0, CTRL('d'), 0},
    {"\"map_view\"", DEMO_CONTEXT, change_mode_func, AUTOMAP_LOOP, 0, CTRL('a'), 0},
    {"\"clear_fullscreen\"", DEMO_CONTEXT, clear_fullscreen_func, 0, 0, DOWN(KEY_BS), 0},
    {"\"save_game\"", DEMO_CONTEXT, saveload_hotkey_func, FALSE, 0, CTRL('s'), 0},
    {"\"load_game\"", DEMO_CONTEXT, saveload_hotkey_func, TRUE, 0, CTRL('l'), 0},
    {"\"pause\"", DEMO_CONTEXT, pause_game_func, TRUE, 0, DOWN('p'), 0},
    {"\"reload_weapon 1\"", DEMO_CONTEXT, reload_weapon_hotkey, 1, 0, CTRL(KEY_BS), 0},
    {"\"reload_weapon 0\"", DEMO_CONTEXT, reload_weapon_hotkey, 0, 0, ALT(KEY_BS), 0},
    {"\"select_grenade\"", DEMO_CONTEXT, select_grenade_hotkey, 0, 0, CTRL('\''), 0},
    {"\"toggle_olh\"", DEMO_CONTEXT, toggle_olh_func, 0, 0, CTRL('h'), 0},
    {"\"select_drug\"", DEMO_CONTEXT, select_drug_hotkey, 0, 0, CTRL(';'), 0},
    {"\"toggle_music\"", DEMO_CONTEXT, toggle_music_func, 0, 0, CTRL('m'), 0},
    {"\"quit\"", DEMO_CONTEXT, demo_quit_func, 0, 0, CTRL('q'), 0},
    {"\"cycle_weapons 1\"", DEMO_CONTEXT, cycle_weapons_func, 1, 0, TAB_KEY, 0},
    {"\"cycle_weapons -1\"", DEMO_CONTEXT, cycle_weapons_func, -1, 0, S_TAB_KEY, 0},
    {"\"cycle_detail\"", DEMO_CONTEXT, MacDetailFunc, 0, 0, CTRL('1'), 0},
    {"\"toggle_opengl\"", EVERY_CONTEXT, toggle_opengl_func, 0, 0, CTRL('g'), 0},
    {"\"arm_grenade\"", DEMO_CONTEXT, arm_grenade_hotkey, 0, 0, ALT('\''), 0},
    {"\"use_drug\"", DEMO_CONTEXT, use_drug_hotkey, 0, 0, ALT(';'), 0},
    {"\"hud_color\"", DEMO_CONTEXT, hud_color_bank_cycle, 0, 0, ALT('h'), 0},
    {"\"showhelp\"", DEMO_CONTEXT, olh_overlay_func, (intptr_t)&olh_overlay_on, 0, ALT('o'), 0},    
    {"\"bio scan\"", DEMO_CONTEXT, hw_hotkey_callback, 5, 0, 49, 0},
    {"\"fullscreen\"", DEMO_CONTEXT, hw_hotkey_callback, 10, 0, 50, 0},
    {"\"360 view\"", DEMO_CONTEXT, hw_hotkey_callback, 2, 0, 51, 0},
    {"\"lantern\"", DEMO_CONTEXT, hw_hotkey_callback, 9, 0, 52, 0},
    {"\"shield\"", DEMO_CONTEXT, hw_hotkey_callback, 7, 0, 53, 0},
    {"\"infrared\"", DEMO_CONTEXT, hw_hotkey_callback, 0, 0, 54, 0},
    {"\"nav unit\"", DEMO_CONTEXT, hw_hotkey_callback, 6, 0, 55, 0},
    {"\"data reader\"", DEMO_CONTEXT, hw_hotkey_callback, 8, 0, 56, 0},
    {"\"booster\"", DEMO_CONTEXT, hw_hotkey_callback, 12, 0, 57, 0},
    {"\"jumpjets\"", DEMO_CONTEXT, hw_hotkey_callback, 13, 0, 48, 0},
    {"\"mfd left 1\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_LEFT, MFD_WEAPON_SLOT), 0, KEY_F1, 0},
    {"\"mfd left 2\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_LEFT, MFD_ITEM_SLOT), 0, KEY_F2, 0},
    {"\"mfd left 3\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_LEFT, MFD_MAP_SLOT), 0, KEY_F3, 0},
    {"\"mfd left 4\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_LEFT, MFD_TARGET_SLOT), 0, KEY_F4, 0},
    {"\"mfd left 5\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_LEFT, MFD_INFO_SLOT), 0, KEY_F5, 0},
    {"\"mfd right 1\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_RIGHT, MFD_WEAPON_SLOT), 0, KEY_F6, 0},
    {"\"mfd right 2\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_RIGHT, MFD_ITEM_SLOT), 0, KEY_F7, 0},
    {"\"mfd right 3\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_RIGHT, MFD_MAP_SLOT), 0, KEY_F8, 0},
    {"\"mfd right 4\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_RIGHT, MFD_TARGET_SLOT), 0, KEY_F9, 0},
    {"\"mfd right 5\"", DEMO_CONTEXT, mfd_button_callback_kb, ENCODE_MFD_SELECTION(MFD_RIGHT, MFD_INFO_SLOT), 0, KEY_F10, 0},
    {"\"keypad 0\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('0'), 0},
    {"\"keypad 1\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('1'), 0},
    {"\"keypad 2\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('2'), 0},
    {"\"keypad 3\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('3'), 0},
    {"\"keypad 4\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('4'), 0},
    {"\"keypad 5\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('5'), 0},
    {"\"keypad 6\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('6'), 0},
    {"\"keypad 7\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('7'), 0},
    {"\"keypad 8\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('8'), 0},
    {"\"keypad 9\"", DEMO_CONTEXT, keypad_hotkey_func, 0, 0, DOWN('9'), 0},
    //  { "\"mac_help\"",         DEMO_CONTEXT, MacHelpFunc,            0                        , 0, CTRL('/'),     0
    //  },
    {"\"toggle_options\"", DEMO_CONTEXT, wrapper_options_func, TRUE, 0, DOWN(KEY_ESC), 0},
    {"\"cheat_give_all\"", DEMO_CONTEXT, toggle_giveall_func, TRUE, 0, CTRL('2'), 0},
    {"\"cheat_physics\"", DEMO_CONTEXT, toggle_physics_func, TRUE, 0, CTRL('3'), 0},
    {"\"cheat_up_level\"", DEMO_CONTEXT, toggle_up_level_func, TRUE, 0, CTRL('4'), 0},
    {"\"cheat_down_level\"", DEMO_CONTEXT, toggle_down_level_func, TRUE, 0, CTRL('5'), 0},

    {NULL, 0, 0, 0}};

// ought to be enough for anybody
#define MAX_FIRE_KEYS 16

int FireKeys[MAX_FIRE_KEYS + 1]; // see input.c

static char *GetKeybindsPathFilename(void) {
    static char filename[512];

    FILE *f = fopen(KEYBINDS_FILENAME, "r");
    if (f != NULL) {
        fclose(f);
        strcpy(filename, KEYBINDS_FILENAME);
    } else {
        char *p = SDL_GetPrefPath("Interrupt", "SystemShock");
        snprintf(filename, sizeof(filename), "%s%s", p, KEYBINDS_FILENAME);
        SDL_free(p);
    }

    return filename;
}

// all hotkey initialization and hotkey_add()s are done in this function
// also handles setting fire keybinds
void LoadHotkeyKeybinds(void) {
    FILE *f;
    char temp[512], *p;
    const char *string;
    int len, i, flags, ch, fire_key_index = 0;

    hotkey_init(NUM_HOTKEYS);

    // clear hotkey used flags so we can tell which weren't specified in file
    // later we can add default key chars for them
    i = 0;
    while (HotKeyLookup[i].s != NULL) {
        HotKeyLookup[i].used = FALSE;
        i++;
    }

    f = fopen(GetKeybindsPathFilename(), "r");
    if (f) {
        // scan keybinds file line by line
        while (fgets(temp, sizeof(temp), f)) {
            LowerCaseInPlace(temp);
            p = temp;

            while (*p && isspace(*p))
                p++; // skip leading spaces

            string = "bind";
            len = strlen(string);
            if (strncmp(p, string, len))
                continue;
            p += len;

            while (*p && isspace(*p))
                p++; // skip leading spaces

            flags = KB_FLAG_DOWN;
            ch = 0;

            string = "shift+";
            len = strlen(string);
            if (!strncmp(p, string, len)) {
                p += len;
                flags |= KB_FLAG_SHIFT;
            }

            string = "ctrl+";
            len = strlen(string);
            if (!strncmp(p, string, len)) {
                p += len;
                flags |= KB_FLAG_CTRL;
            }

            string = "alt+";
            len = strlen(string);
            if (!strncmp(p, string, len)) {
                p += len;
                flags |= KB_FLAG_ALT;
            }

            // get ascii char from key name
            i = 0;
            while (KeyName2ChCode[i].s != NULL) {
                string = KeyName2ChCode[i].s;
                len = strlen(string);
                if (!strncmp(p, string, len)) {
                    p += len;
                    ch = KeyName2ChCode[i].ch | flags;
                    break;
                }
                i++;
            }
            if (ch == 0)
                continue;

            while (*p && isspace(*p))
                p++; // skip leading spaces

            // lookup and add specified hotkey info
            i = 0;
            while (HotKeyLookup[i].s != NULL) {
                string = HotKeyLookup[i].s;
                len = strlen(string);
                if (!strncmp(p, string, len)) {
                    hotkey_add(ch, HotKeyLookup[i].contexts, HotKeyLookup[i].func, HotKeyLookup[i].state);
                    HotKeyLookup[i].used = TRUE;
                    break;
                }
                i++;
            }

            // special case for fire keys
            string = "\"fire\"";
            len = strlen(string);
            if (!strncmp(p, string, len)) {
                if (fire_key_index < MAX_FIRE_KEYS)
                    FireKeys[fire_key_index++] = (ch & ~KB_FLAG_DOWN);
            }
        }

        fclose(f);
    }

    // add defaults for unused hotkeys
    i = 0;
    while (HotKeyLookup[i].s != NULL) {
        if (!HotKeyLookup[i].used) {
            // add default 1
            ch = HotKeyLookup[i].def1;
            if (ch)
                hotkey_add(ch, HotKeyLookup[i].contexts, HotKeyLookup[i].func, HotKeyLookup[i].state);

            // add default 2
            ch = HotKeyLookup[i].def2;
            if (ch)
                hotkey_add(ch, HotKeyLookup[i].contexts, HotKeyLookup[i].func, HotKeyLookup[i].state);
        }
        i++;
    }

    // add default fire key if none were specified
    if (fire_key_index == 0)
        FireKeys[fire_key_index++] = KEY_ENTER;

    // signal end of fire key list
    FireKeys[fire_key_index] = 0;
}

//************************************************************************************

//**********************************
// Set move keybinds (see movekeys.c)
//**********************************

extern MOVE_KEYBIND MoveKeybinds[MAX_MOVE_KEYBINDS + 1];
extern MOVE_KEYBIND MoveCyberKeybinds[MAX_MOVE_KEYBINDS + 1];

static MOVE_KEYBIND MoveKeybindsDefault[] =
{
  { CODE_W        | KB_FLAG_SHIFT, M_RUNFORWARD    },
  { CODE_UP       | KB_FLAG_SHIFT, M_RUNFORWARD    },
  { CODE_UP       | KB_FLAG_ALT  , M_RUNFORWARD    },
  { CODE_KP_UP    | KB_FLAG_SHIFT, M_RUNFORWARD    },
  { CODE_KP_UP    | KB_FLAG_ALT  , M_RUNFORWARD    },
  { CODE_W                       , M_FORWARD       },
  { CODE_UP                      , M_FORWARD       },
  { CODE_KP_UP                   , M_FORWARD       },
  { CODE_Z        | KB_FLAG_SHIFT, M_FASTTURNLEFT  },
  { CODE_LEFT     | KB_FLAG_SHIFT, M_FASTTURNLEFT  },
  { CODE_KP_LEFT  | KB_FLAG_SHIFT, M_FASTTURNLEFT  },
  { CODE_Z                       , M_TURNLEFT      },
  { CODE_LEFT                    , M_TURNLEFT      },
  { CODE_KP_LEFT                 , M_TURNLEFT      },
  { CODE_C        | KB_FLAG_SHIFT, M_FASTTURNRIGHT },
  { CODE_RIGHT    | KB_FLAG_SHIFT, M_FASTTURNRIGHT },
  { CODE_KP_RIGHT | KB_FLAG_SHIFT, M_FASTTURNRIGHT },
  { CODE_C                       , M_TURNRIGHT     },
  { CODE_RIGHT                   , M_TURNRIGHT     },
  { CODE_KP_RIGHT                , M_TURNRIGHT     },
  { CODE_S                       , M_BACK          },
  { CODE_S        | KB_FLAG_SHIFT, M_BACK          },
  { CODE_DOWN                    , M_BACK          },
  { CODE_DOWN     | KB_FLAG_SHIFT, M_BACK          },
  { CODE_DOWN     | KB_FLAG_ALT  , M_BACK          },
  { CODE_KP_DOWN                 , M_BACK          },
  { CODE_KP_DOWN  | KB_FLAG_SHIFT, M_BACK          },
  { CODE_KP_DOWN  | KB_FLAG_ALT  , M_BACK          },
  { CODE_A                       , M_SLIDELEFT     },
  { CODE_A        | KB_FLAG_SHIFT, M_SLIDELEFT     },
  { CODE_LEFT     | KB_FLAG_ALT  , M_SLIDELEFT     },
  { CODE_KP_LEFT  | KB_FLAG_ALT  , M_SLIDELEFT     },
  { CODE_KP_END                  , M_SLIDELEFT     },
  { CODE_D                       , M_SLIDERIGHT    },
  { CODE_D        | KB_FLAG_SHIFT, M_SLIDERIGHT    },
  { CODE_RIGHT    | KB_FLAG_ALT  , M_SLIDERIGHT    },
  { CODE_KP_RIGHT | KB_FLAG_ALT  , M_SLIDERIGHT    },
  { CODE_KP_PGDN                 , M_SLIDERIGHT    },
  { CODE_J                       , M_JUMP          },
  { CODE_J        | KB_FLAG_SHIFT, M_JUMP          },
  { CODE_SPACE                   , M_JUMP          },
  { CODE_SPACE    | KB_FLAG_SHIFT, M_JUMP          },
  { CODE_SPACE    | KB_FLAG_CTRL , M_JUMP          },
  { CODE_SPACE    | KB_FLAG_ALT  , M_JUMP          },
  { CODE_X                       , M_LEANUP        },
  { CODE_X        | KB_FLAG_SHIFT, M_LEANUP        },
  { CODE_X        | KB_FLAG_CTRL , M_LEANUP        },
  { CODE_X        | KB_FLAG_ALT  , M_LEANUP        },
  { CODE_Q                       , M_LEANLEFT      },
  { CODE_Q        | KB_FLAG_SHIFT, M_LEANLEFT      },
  { CODE_LEFT     | KB_FLAG_CTRL , M_LEANLEFT      },
  { CODE_KP_LEFT  | KB_FLAG_CTRL , M_LEANLEFT      },
  { CODE_E                       , M_LEANRIGHT     },
  { CODE_E        | KB_FLAG_SHIFT, M_LEANRIGHT     },
  { CODE_RIGHT    | KB_FLAG_CTRL , M_LEANRIGHT     },
  { CODE_KP_RIGHT | KB_FLAG_CTRL , M_LEANRIGHT     },
  { CODE_R                       , M_LOOKUP        },
  { CODE_R        | KB_FLAG_SHIFT, M_LOOKUP        },
  { CODE_R        | KB_FLAG_CTRL , M_LOOKUP        },
  { CODE_UP       | KB_FLAG_CTRL , M_LOOKUP        },
  { CODE_KP_UP    | KB_FLAG_CTRL , M_LOOKUP        },
  { CODE_V                       , M_LOOKDOWN      },
  { CODE_V        | KB_FLAG_SHIFT, M_LOOKDOWN      },
  { CODE_V        | KB_FLAG_CTRL , M_LOOKDOWN      },
  { CODE_DOWN     | KB_FLAG_CTRL , M_LOOKDOWN      },
  { CODE_KP_DOWN  | KB_FLAG_CTRL , M_LOOKDOWN      },
  { CODE_KP_HOME                 , M_RUNLEFT       },
  { CODE_KP_PGUP                 , M_RUNRIGHT      },
  { CODE_S                       , M_THRUST        }, //cyber start
  { CODE_S        | KB_FLAG_SHIFT, M_THRUST        },
  { CODE_KP_5                    , M_THRUST        },
  { CODE_W                       , M_CLIMB         },
  { CODE_W        | KB_FLAG_SHIFT, M_CLIMB         },
  { CODE_UP                      , M_CLIMB         },
  { CODE_UP       | KB_FLAG_SHIFT, M_CLIMB         },
  { CODE_UP       | KB_FLAG_CTRL , M_CLIMB         },
  { CODE_UP       | KB_FLAG_ALT  , M_CLIMB         },
  { CODE_KP_UP                   , M_CLIMB         },
  { CODE_KP_UP    | KB_FLAG_SHIFT, M_CLIMB         },
  { CODE_KP_UP    | KB_FLAG_CTRL , M_CLIMB         },
  { CODE_KP_UP    | KB_FLAG_ALT  , M_CLIMB         },
  { CODE_A                       , M_BANKLEFT      },
  { CODE_A        | KB_FLAG_SHIFT, M_BANKLEFT      },
  { CODE_KP_LEFT                 , M_BANKLEFT      },
  { CODE_KP_LEFT  | KB_FLAG_SHIFT, M_BANKLEFT      },
  { CODE_KP_LEFT  | KB_FLAG_CTRL , M_BANKLEFT      },
  { CODE_KP_LEFT  | KB_FLAG_ALT  , M_BANKLEFT      },
  { CODE_D                       , M_BANKRIGHT     },
  { CODE_D        | KB_FLAG_SHIFT, M_BANKRIGHT     },
  { CODE_KP_RIGHT                , M_BANKRIGHT     },
  { CODE_KP_RIGHT | KB_FLAG_SHIFT, M_BANKRIGHT     },
  { CODE_KP_RIGHT | KB_FLAG_CTRL , M_BANKRIGHT     },
  { CODE_KP_RIGHT | KB_FLAG_ALT  , M_BANKRIGHT     },
  { CODE_X                       , M_DIVE          },
  { CODE_X        | KB_FLAG_SHIFT, M_DIVE          },
  { CODE_DOWN                    , M_DIVE          },
  { CODE_DOWN     | KB_FLAG_SHIFT, M_DIVE          },
  { CODE_DOWN     | KB_FLAG_CTRL , M_DIVE          },
  { CODE_DOWN     | KB_FLAG_ALT  , M_DIVE          },
  { CODE_KP_DOWN                 , M_DIVE          },
  { CODE_KP_DOWN  | KB_FLAG_SHIFT, M_DIVE          },
  { CODE_KP_DOWN  | KB_FLAG_CTRL , M_DIVE          },
  { CODE_KP_DOWN  | KB_FLAG_ALT  , M_DIVE          },
  { CODE_Q                       , M_ROLLRIGHT     },
  { CODE_Q        | KB_FLAG_SHIFT, M_ROLLRIGHT     },
  { CODE_Z                       , M_ROLLRIGHT     },
  { CODE_Z        | KB_FLAG_SHIFT, M_ROLLRIGHT     },
  { CODE_E                       , M_ROLLLEFT      },
  { CODE_E        | KB_FLAG_SHIFT, M_ROLLLEFT      },
  { CODE_C                       , M_ROLLLEFT      },
  { CODE_C        | KB_FLAG_SHIFT, M_ROLLLEFT      },
  { CODE_KP_HOME                 , M_CLIMBLEFT     },
  { CODE_KP_HOME  | KB_FLAG_SHIFT, M_CLIMBLEFT     },
  { CODE_KP_HOME  | KB_FLAG_CTRL , M_CLIMBLEFT     },
  { CODE_KP_HOME  | KB_FLAG_ALT  , M_CLIMBLEFT     },
  { CODE_KP_PGUP                 , M_CLIMBRIGHT    },
  { CODE_KP_PGUP  | KB_FLAG_SHIFT, M_CLIMBRIGHT    },
  { CODE_KP_PGUP  | KB_FLAG_CTRL , M_CLIMBRIGHT    },
  { CODE_KP_PGUP  | KB_FLAG_ALT  , M_CLIMBRIGHT    },
  { CODE_KP_PGDN                 , M_DIVERIGHT     },
  { CODE_KP_PGDN  | KB_FLAG_SHIFT, M_DIVERIGHT     },
  { CODE_KP_PGDN  | KB_FLAG_CTRL , M_DIVERIGHT     },
  { CODE_KP_PGDN  | KB_FLAG_ALT  , M_DIVERIGHT     },
  { CODE_KP_END                  , M_DIVELEFT      },
  { CODE_KP_END   | KB_FLAG_SHIFT, M_DIVELEFT      },
  { CODE_KP_END   | KB_FLAG_CTRL , M_DIVELEFT      },
  { CODE_KP_END   | KB_FLAG_ALT  , M_DIVELEFT      },

                                             {255, -1}};

static struct {
    const char *s;
    int move;
} MoveName2Move[] = {{"\"runforward\"", M_RUNFORWARD},
                     {"\"forward\"", M_FORWARD},
                     {"\"fastturnleft\"", M_FASTTURNLEFT},
                     {"\"turnleft\"", M_TURNLEFT},
                     {"\"fastturnright\"", M_FASTTURNRIGHT},
                     {"\"turnright\"", M_TURNRIGHT},
                     {"\"back\"", M_BACK},
                     {"\"slideleft\"", M_SLIDELEFT},
                     {"\"slideright\"", M_SLIDERIGHT},
                     {"\"jump\"", M_JUMP},
                     {"\"leanup\"", M_LEANUP},
                     {"\"leanleft\"", M_LEANLEFT},
                     {"\"leanright\"", M_LEANRIGHT},
                     {"\"lookup\"", M_LOOKUP},
                     {"\"lookdown\"", M_LOOKDOWN},
                     {"\"runleft\"", M_RUNLEFT},
                     {"\"runright\"", M_RUNRIGHT},
                     {"\"thrust\"", M_THRUST}, // cyber start
                     {"\"climb\"", M_CLIMB},
                     {"\"bankleft\"", M_BANKLEFT},
                     {"\"bankright\"", M_BANKRIGHT},
                     {"\"dive\"", M_DIVE},
                     {"\"rollright\"", M_ROLLRIGHT},
                     {"\"rollleft\"", M_ROLLLEFT},
                     {"\"climbleft\"", M_CLIMBLEFT},
                     {"\"climbright\"", M_CLIMBRIGHT},
                     {"\"diveright\"", M_DIVERIGHT},
                     {"\"diveleft\"", M_DIVELEFT},

                     {NULL, 0}};

void LoadMoveKeybinds(void) {
    FILE *f;
    char temp[512], *p, move_used[NUM_MOVES];
    const char *string;
    int len, i, flags, code, move, num_bound = 0, num_cyber_bound = 0;

    // keep track of which moves are specified so we can add default ones for those that are missing
    memset(move_used, 0, NUM_MOVES);

    f = fopen(GetKeybindsPathFilename(), "r");
    if (f) {
        // scan keybinds file line by line
        while (fgets(temp, sizeof(temp), f)) {
            LowerCaseInPlace(temp);
            p = temp;

            while (*p && isspace(*p))
                p++; // skip leading spaces

            string = "bind";
            len = strlen(string);
            if (strncmp(p, string, len))
                continue;
            p += len;

            while (*p && isspace(*p))
                p++; // skip leading spaces

            flags = 0;
            code = 255;

            string = "shift+";
            len = strlen(string);
            if (!strncmp(p, string, len)) {
                p += len;
                flags |= KB_FLAG_SHIFT;
            }

            string = "ctrl+";
            len = strlen(string);
            if (!strncmp(p, string, len)) {
                p += len;
                flags |= KB_FLAG_CTRL;
            }

            string = "alt+";
            len = strlen(string);
            if (!strncmp(p, string, len)) {
                p += len;
                flags |= KB_FLAG_ALT;
            }

            // get code from key name
            i = 0;
            while (KeyName2ChCode[i].s != NULL) {
                string = KeyName2ChCode[i].s;
                len = strlen(string);
                if (!strncmp(p, string, len)) {
                    p += len;
                    code = KeyName2ChCode[i].code | flags;
                    break;
                }
                i++;
            }
            if (code == 255)
                continue;

            while (*p && isspace(*p))
                p++; // skip leading spaces

            // lookup move
            i = 0;
            while (MoveName2Move[i].s != NULL) {
                string = MoveName2Move[i].s;
                len = strlen(string);
                if (!strncmp(p, string, len)) {
                    move = MoveName2Move[i].move;
                    move_used[move] = 1;

                    if (move < M_THRUST) // non-cyber
                    {
                        if (num_bound < MAX_MOVE_KEYBINDS) {
                            // add keybind to list
                            MoveKeybinds[num_bound].code = code;
                            MoveKeybinds[num_bound].move = move;
                            num_bound++;
                        }
                    } else {
                        if (num_cyber_bound < MAX_MOVE_KEYBINDS) {
                            // add cyber keybind to list
                            MoveCyberKeybinds[num_cyber_bound].code = code;
                            MoveCyberKeybinds[num_cyber_bound].move = move;
                            num_cyber_bound++;
                        }
                    }

                    break;
                }
                i++;
            }
        }

        fclose(f);
    }

    // for moves that weren't referenced in file, bind default codes to them
    for (move = 0; move < NUM_MOVES; move++)
        if (!move_used[move]) {
            i = 0;
            while (MoveKeybindsDefault[i].code != 255) {
                if (MoveKeybindsDefault[i].move == move) {
                    code = MoveKeybindsDefault[i].code;

                    if (move < M_THRUST) // non-cyber
                    {
                        if (num_bound < MAX_MOVE_KEYBINDS) {
                            // add keybind to list
                            MoveKeybinds[num_bound].code = code;
                            MoveKeybinds[num_bound].move = move;
                            num_bound++;
                        }
                    } else {
                        if (num_cyber_bound < MAX_MOVE_KEYBINDS) {
                            // add cyber keybind to list
                            MoveCyberKeybinds[num_cyber_bound].code = code;
                            MoveCyberKeybinds[num_cyber_bound].move = move;
                            num_cyber_bound++;
                        }
                    }
                }
                i++;
            }
        }

    // signal end of lists
    MoveKeybinds[num_bound].code = 255;
    MoveCyberKeybinds[num_cyber_bound].code = 255;

    extern void init_motion_polling(void); // see movekeys.c
    init_motion_polling();
}

//************************************************************************************

//*******************************
// Create default keybinds file
//*******************************

#define JUSTIFY_COLUMN 30

// if ch is 0, use code instead
static bool WriteKeyName(int ch, int code, FILE *f) {
    int i = 0, len = 0;

    while (KeyName2ChCode[i].s != NULL) {
        if (ch) {
            if (KeyName2ChCode[i].ch == (ch & 255))
                break;
        } else {
            if (KeyName2ChCode[i].code == (code & 255))
                break;
        }
        i++;
    }
    if (KeyName2ChCode[i].s == NULL)
        return 0;

    fputs("bind  ", f);
    len += 6;

    if ((ch ? ch : code) & KB_FLAG_SHIFT) {
        fputs("shift+", f);
        len += 6;
    }
    if ((ch ? ch : code) & KB_FLAG_CTRL) {
        fputs("ctrl+", f);
        len += 5;
    }
    if ((ch ? ch : code) & KB_FLAG_ALT) {
        fputs("alt+", f);
        len += 4;
    }

    fputs(KeyName2ChCode[i].s, f);
    len += strlen(KeyName2ChCode[i].s);

    // add spaces to justify following text
    while (len < JUSTIFY_COLUMN) {
        fputc(' ', f);
        len++;
    }

    return 1;
}

static void WriteMoveName(int move, FILE *f) {
    int i;

    // find move name that matches move
    i = 0;
    while (MoveName2Move[i].s != NULL) {
        if (MoveName2Move[i].move == move)
            break;
        i++;
    }

    if (MoveName2Move[i].s != NULL) {
        fputs(MoveName2Move[i].s, f);
        fputs("\n", f);
    }
}

// create default keybinds file if it doesn't already exist
void CreateDefaultKeybindsFile(void) {
    FILE *f;
    char *filename = GetKeybindsPathFilename();
    int i, ch;

    // check if file already exists; if so, return
    f = fopen(filename, "r");
    if (f != NULL) {
        fclose(f);
        return;
    }

    // open new file for writing
    f = fopen(filename, "w");
    if (f == NULL)
        return;

    // write default hotkey keybinds
    i = 0;
    while (HotKeyLookup[i].s) {
        // default 1 if it exists (it should)
        ch = HotKeyLookup[i].def1;
        if (ch && WriteKeyName(ch, 0, f)) {
            fputs(HotKeyLookup[i].s, f);
            fputs("\n", f);
        }

        // default 2 if it exists (it might not)
        ch = HotKeyLookup[i].def2;
        if (ch && WriteKeyName(ch, 0, f)) {
            fputs(HotKeyLookup[i].s, f);
            fputs("\n", f);
        }

        i++;
    }

    // write default fire keybind
    fputs("\n", f);
    WriteKeyName(KEY_ENTER, 0, f);
    fputs("\"fire\"\n\n", f);

    // write default move keybinds
    i = 0;
    while (MoveKeybindsDefault[i].code != 255) {
        if (WriteKeyName(0, MoveKeybindsDefault[i].code, f))
            WriteMoveName(MoveKeybindsDefault[i].move, f);

        i++;
    }

    fclose(f);
}
