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

#include "movie.h"
#include "res.h"

Movie *MoviePrepareRes(Id id, uint8_t *buff, int32_t buffLen, int32_t blockLen) {
    Movie *result;

    DEBUG("%s: opening media", __FUNCTION__);

    return result;
}

/**
 * Play movie
 * @param pmovie pointer to Movie
 * @param pcanvas canvas to show video, NULL if you need to play only audio
 */
void MoviePlay(Movie *pmovie, grs_canvas *pcanvas) {

    DEBUG("%s: playing media", __FUNCTION__);

    return;
}
