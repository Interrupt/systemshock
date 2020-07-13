/*

Copyright (C) 2018-2020 Shockolate Project

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

#ifndef __MODDING_H
#define __MODDING_H

//--------------------
// Defines
//--------------------

#define MAX_MOD_FILES 100

//--------------------
// Public Globals
//--------------------

// Let people override the default game archive
extern char *modding_archive_override;

// Additional resource files to load
extern char *modding_additional_files[MAX_MOD_FILES];

extern int num_mod_files;

//--------------------
//  Function Prototypes
//--------------------

int AddResourceFile(char *filename);
int AddArchiveFile(char *filename);
int ProcessModFile(char *filename, uchar follow_dirs);
int ProcessModArgs(int argc, char **argv);
int ProcessModDirectory(char *dirname);
int LoadModFiles(void);

#endif //__MODDING_H
