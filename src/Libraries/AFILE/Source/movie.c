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

#include <stdio.h>

#include "afile.h"
#include "movie.h"
#include "res.h"

FILE *temp_file;

int32_t AfilePrepareRes(Id id, Afile *afile) {

    uint8_t *ptr = ResLock(id);

    // FIXME That's ugly. We need fmemopen() analog.
    temp_file = tmpfile();
    fwrite(ptr, ResSize(id), 1, temp_file);
    fseek(temp_file, 0, 0);

    ResUnlock(id);

    int32_t error = AfileOpen(afile, temp_file, AFILE_MOV);

    return error;
}
