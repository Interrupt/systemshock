/*

Copyright (C) 2018 Shockolate Project

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

// DG 2018: a case-insensitive fopen() wrapper, and functions used by it

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

size_t DG_strlcpy(char *dst, const char *src, size_t dstsize) {
    assert(src && dst && "Don't call strlcpy with NULL arguments!");
    size_t srclen = strlen(src);

    if (dstsize != 0) {
        size_t numchars = dstsize - 1;

        if (srclen < numchars)
            numchars = srclen;

        memcpy(dst, src, numchars);
        dst[numchars] = '\0';
    }
    return srclen;
}

size_t DG_strlcat(char *dst, const char *src, size_t dstsize) {
    assert(src && dst && "Don't call strlcat with NULL arguments!");

    size_t dstlen = strnlen(dst, dstsize);
    size_t srclen = strlen(src);

    assert(dstlen != dstsize && "dst must contain null-terminated data with strlen < dstsize!");

    // TODO: dst[dstsize-1] = '\0' to ensure null-termination and make wrong dstsize more obvious?

    if (dstsize > 1 && dstlen < dstsize - 1) {
        size_t numchars = dstsize - dstlen - 1;

        if (srclen < numchars)
            numchars = srclen;

        memcpy(dst + dstlen, src, numchars);
        dst[dstlen + numchars] = '\0';
    }

    return dstlen + srclen;
}

#ifndef _WIN32

#include <dirent.h>
#include <unistd.h>

static int check_and_append_pathelem(char dirbuf[PATH_MAX], const char *elem) {
    DIR *basedir = opendir(dirbuf);
    int ret = 0;
    if (basedir != NULL) {
        struct dirent *entry;
        for (entry = readdir(basedir); entry != NULL; entry = readdir(basedir)) {
            if (strcasecmp(entry->d_name, elem) == 0) {
                size_t dblen = strlen(dirbuf);
                if (dirbuf[dblen - 1] != '/') {
                    dirbuf[dblen] = '/';
                    dirbuf[dblen + 1] = '\0';
                }
                DG_strlcat(dirbuf, entry->d_name, PATH_MAX);

                ret = 1;
                break;
            }
        }

        closedir(basedir);
    }
    return ret;
}
#endif // not _WIN32

// checks if a version of file with path inpath with different case exists.
// if so, the corrected version is copied to outpath.
// => outpath must be able to hold at least strlen(inpath)+2 chars.
// if wantdir is 1, the path must lead to a directory
//   if it's 0, it must be a file
//   if it's -1 it can be either (unless inpath ends with '/' then it must be a directory)
// returns 1 if the file (or directory) could be found, 0 if not
int caselesspath(const char *inpath, char *outpath, int wantdir) {
    size_t inlen = strlen(inpath);

#ifdef _WIN32

    // windows is case insensitive, just do a stat()
    struct _stat statBuf;
    int isdir = 0;

    outpath[0] = '\0';

    if (inlen == 0)
        return 0;

    if (inpath[inlen - 1] == '/' || inpath[inlen - 1] == '\\') {
        if (wantdir == 0)
            return 0; // if it ends with a /, it's no file
        else
            wantdir = 1;
    }

    if (_stat(inpath, &statBuf) != 0)
        return 0;

    isdir = (statBuf.st_mode & _S_IFDIR) != 0;
    if (wantdir == -1 || isdir == wantdir) {
        DG_strlcpy(outpath, inpath, inlen + 1);
        return 1;
    }
    return 0;

#else // not Windows - more complicated

    // anyway, first do the cheap check with a stat(), maybe the case already is correct
    struct stat statBuf;

    outpath[0] = '\0';

    if (inlen == 0)
        return 0;

    if (inpath[inlen - 1] == '/') {
        if (wantdir == 0)
            return 0; // if it ends with a /, it's no file
        else
            wantdir = 1;
    }

    if (stat(inpath, &statBuf) == 0) {
        // the file exists, now we only need to make sure it's a directory
        // or not, depending on isdir
        int isdir = ((statBuf.st_mode & S_IFDIR) != 0);
        if (wantdir == -1 || isdir == wantdir) {
            DG_strlcpy(outpath, inpath, inlen + 1);
            return 1;
        }
        return 0;
    } else // not found with stat, do it the hard way
    {
        char *curdirtok = NULL;
        char *strtokctx = NULL;
        const char *orig_inpath = inpath;

        char dirbuf[PATH_MAX] = {0};
        char inpathcpy[PATH_MAX] = {0};

        outpath[0] = '\0';

        if (inpath[0] == '/') {
            dirbuf[0] = '/';
            dirbuf[1] = '\0';
            ++inpath;
        } else if (inpath[0] == '.' && inpath[1] == '.') {
            if (inpath[2] != '/') {
                return 0; // malformed path, starting with .. but not ../
            }
            DG_strlcpy(dirbuf, "..", 3);
            inpath += 2;
        } else {
            dirbuf[0] = '.';
            dirbuf[1] = '/';
            //++inpath;
        }

        if (DG_strlcpy(inpathcpy, inpath, sizeof(inpathcpy)) >= sizeof(inpathcpy)) {
            // sorry, path too long
            return 0;
        }

        for (curdirtok = strtok_r(inpathcpy, "/", &strtokctx); curdirtok != NULL;
             curdirtok = strtok_r(NULL, "/", &strtokctx)) {
            // if the path contained /./ just ignore that
            if (curdirtok[0] == '.' && curdirtok[1] == '\0')
                continue;

            if (!check_and_append_pathelem(dirbuf, curdirtok)) {
                // ok, that element couldn't be found
                return 0;
            }
        }

        // now do a stat() to make sure the whole thing matches wantdir
        // FIXME: somehow the stat() destroys dirbuf(), even though that really shouldn't happen..
        // if(stat(dirbuf, &statBuf) == 0)
        {
            // the file exists, now we only need to make sure it's a directory
            // or not, depending on isdir
            // int isdir = ((statBuf.st_mode & S_IFDIR) != 0);
            // if(wantdir != -1 && isdir != wantdir)  return 0;

            if (dirbuf[0] == '/') {
                assert(strlen(dirbuf) <= inlen && "the output string shouldn't be longer than input!");
                DG_strlcpy(outpath, dirbuf, inlen + 1);
            } else {
                size_t outoffset = 0;
                // assert(strlen(dirbuf+outoffset) <= inlen && "the output string shouldn't be longer than input!");
                // if the orig string didn't start with "./", skip that for the output as well
                if (orig_inpath[0] != '.' || orig_inpath[1] != '/')
                    outoffset = 2;
                DG_strlcpy(outpath, dirbuf + outoffset, inlen + 1);
            }
            if (orig_inpath[inlen - 1] == '/') {
                // restore the trailing '/' that has been eaten by strtok_r()
                DG_strlcat(outpath, "/", inlen + 1);
            }

            return 1;
        }

        return 0;
    }

#endif // not Windows
}

FILE *fopen_caseless(const char *path, const char *mode) {
    FILE *ret = NULL;

    if (path == NULL || mode == NULL)
        return NULL;

    ret = fopen(path, mode);

#ifndef _WIN32 // not windows
    if (ret == NULL) {
        char fixedpath[PATH_MAX];
        size_t pathlen = strlen(path);

        if (pathlen < sizeof(fixedpath) && caselesspath(path, fixedpath, 0)) {
            ret = fopen(fixedpath, mode);
        }
    }
#endif // not windows

    return ret;
}
