
#include "Modding.h"
#include "lg.h"

#include <stdio.h>
#include <dirent.h>

int StringEndsWith( char *src, char *dst) {
	char* s = strrchr(src, '.');

	if(s != NULL)
		return(strcmp(s, dst)) == 0;

	return 0;
}

int AddResourceFile(char* filename) {
	printf("Found resource file: %s\n", filename);

	if(num_mod_files < MAX_MOD_FILES) {
		char *f = (char*)malloc(strlen(filename) + 1);
		strcpy(f, filename);
		modding_additional_files[num_mod_files++] = f;
	}

	return OK;
}

int AddArchiveFile(char* filename) {
	printf("Found archive file: %s\n", filename);

	char *f = (char*)malloc(strlen(filename) + 1);
	strcpy(f, filename);
	modding_archive_override = f;
	return OK;
}

int ProcessModFile(char* filename, uchar follow_dirs) {
	printf("ProcessModFile: %s\n", filename);

	if(StringEndsWith(filename, ".dat")) {
		AddArchiveFile(filename);
	}
	else if(StringEndsWith(filename, ".res")) {
		AddResourceFile(filename);
	}
	else if(follow_dirs) {
		ProcessModDirectory(filename);
	}
	return OK;
}

int ProcessModArgs(int argc, char** argv) {
	// Default things
	num_mod_files = 0;
	int mod_args_start = 1;

	// Skip arguments
	for(int i = 1; i < argc; i++) {
		if(argv[i][0] == '-') {
			mod_args_start++;
		}
	}

	// Default the mod list to empty
	modding_archive_override = NULL;
	for(int i = 0; i < MAX_MOD_FILES; i++) {
		modding_additional_files[i] = NULL;
	}

	// Now go process args
	for(int i = mod_args_start; i < argc; i++) {
		ProcessModFile(argv[i], TRUE);
	}
}

int ProcessModDirectory(char* dirname) {
	// Check if this is a directory

	printf("ProcessModDirectory %s\n", dirname);

	DIR *dp = opendir(dirname);
	if(dp != NULL) {
		struct dirent *ep;

		// Loop through all files here, calling ProcessModFile for each
		while(ep = readdir(dp)) {

			printf("ep->d_name %s\n", ep->d_name);
			char buf[strlen(dirname) + strlen(ep->d_name) + 2];

			strcpy(buf, dirname);

			// Windows, why do you have to be weird?
			#ifdef _WIN32
			strcat(buf, "\\");
			#else
			strcat(buf, "/");
			#endif

			strcat(buf, ep->d_name);

			ProcessModFile(buf, FALSE);
		}
		
		closedir(dp);
	}
}

int LoadModFiles() {
	for(int i = 0; i < num_mod_files; i++) {
		if(modding_additional_files[i] != NULL) {
			printf("Loading mod file %s\n", modding_additional_files[i]);
			ResOpenFile(modding_additional_files[i]);
		}
	}
}
