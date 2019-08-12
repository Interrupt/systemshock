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
 * $Source: r:/prj/cit/src/RCS/gamestrn.c $
 * $Revision: 1.17 $
 * $Author: xemu $
 * $Date: 1994/09/14 19:35:47 $
 *
 */

#include <string.h>

#include "Shock.h"
#include "gamestrn.h"
#include "criterr.h"
#include "objsim.h"
#include "objprop.h"
#include "cybstrng.h"

// -------------------
// FILE NAMES AND VARS
// -------------------

#define FILENAME_LEN 14
#define CONFIG_STRING_RES_FILE "cfgstrn.res"
#define DEFAULT_STRING_RES_FILE "cybstrng.rsrc"
#define STRING_RES_VAR "strings"

// -------
// GLOBALS
// -------

int string_res_file; // string res filenum

// ---------
// EXTERNALS
// ---------

uchar *language_files[] = {"res/data/cybstrng.res", "res/data/frnstrng.res", "res/data/gerstrng.res"};
char which_lang;

// Wrapper around RefGet suitable for use by lg_sprintf to get string resources
// for the custom '%S' format specifier.
char *lg_sprintf_string_get(uint32_t ref)
{
    return RefGetRaw(ref);
}

void init_strings(void) {
    // Open the string resource file.
    if (which_lang < 0 || which_lang >= sizeof(language_files) / sizeof(*language_files))
        which_lang = 0;
    const uchar *lang_file = language_files[which_lang];
    string_res_file = ResOpenFile(lang_file);

    if (string_res_file < 0)
        critical_error(CRITERR_RES | 0);

    lg_sprintf_install_stringfunc(lg_sprintf_string_get);
}

char *get_string(int num, char *buf, int bufsize) {
    RefTable *table = RefTableGet(REFID(num));
    if (!ResInUse(REFID(num)) || !RefIndexValid(table, REFINDEX(num))) {
        if (buf != NULL) {
            *buf = '\0';
            return buf;
        } else
            return "";
    }
    if (buf != NULL) {
        char *s = (char *)RefLockRaw(num);
        if (s != NULL) {
            strncpy(buf, s, bufsize);
            buf[bufsize - 1] = '\0';

            // printf("Got string %s\n", buf);
        }
        RefUnlock(num);
        return (s == NULL) ? NULL : buf;
    } else
        return get_temp_string(num);
}

char *get_temp_string(int num) { return (char *)RefGetRaw(num); }

char *get_object_short_name(int trip, char *buf, int bufsize) {
    return get_string(MKREF(RES_objshortnames, OPTRIP(trip)), buf, bufsize);
}

char *get_object_long_name(int trip, char *buf, int bufsize) {
    return get_string(MKREF(RES_objlongnames, OPTRIP(trip)), buf, bufsize);
}

void shutdown_strings(void) { ResCloseFile(string_res_file); }

char *get_texture_name(int abs_texture, char *buf, int bufsiz) {
    return get_string(MKREF(RES_texnames, abs_texture), buf, bufsiz);
}

char *get_texture_use_string(int abs_texture, char *buf, int bufsiz) {
    return get_string(MKREF(RES_texuse, abs_texture), buf, bufsiz);
}
