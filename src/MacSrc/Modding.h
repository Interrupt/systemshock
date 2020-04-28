
#ifndef __MODDING_H
#define __MODDING_H

//--------------------
// Defines
//--------------------

#define MAX_MOD_FILES		100

//--------------------
// Public Globals
//--------------------

// Let people override the default game archive
extern char* modding_archive_override;

// Additional resource files to load
extern char* modding_additional_files[MAX_MOD_FILES];

extern int num_mod_files;

//--------------------
//  Function Prototypes
//--------------------

int AddResourceFile(char* filename);
int AddArchiveFile(char* filename);
int ProcessModFile(char* filename, uchar follow_dirs);
int ProcessModArgs(int argc, char** argv);
int ProcessModDirectory(char* dirname);
int LoadModFiles(void);

#endif //__MODDING_H
