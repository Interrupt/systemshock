/*

Copyright (C) 2019 Shockolate Project

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
#include "res.h"

#include "grs.h" // grs_font

// Describe a font.
// FIXME treats the offsets table as raw, should be decoded also.
const ResLayout FontLayout = {
    84,                           // size on disc (header only)
    offsetof(grs_font, off_tab),  // size in memory (header only)
    LAYOUT_FLAG_RAW_DATA_FOLLOWS, // flags
    {
	{ RFFT_UINT16, offsetof(grs_font, id)      },
	{ RFFT_PAD,    34 }, // dummy1
	{ RFFT_UINT16, offsetof(grs_font, min)     },
	{ RFFT_UINT16, offsetof(grs_font, max)     },
	{ RFFT_PAD,    32 }, // dummy2
	{ RFFT_UINT32, offsetof(grs_font, cotptr)  },
	{ RFFT_UINT32, offsetof(grs_font, buf)     },
	{ RFFT_UINT16, offsetof(grs_font, w)       },
	{ RFFT_UINT16, offsetof(grs_font, h)       },
	{ RFFT_RAW,    offsetof(grs_font, off_tab) },
	{ RFFT_END,    0 }
	// offsets table follows, then bitmap data
    }
};

const ResourceFormat FontFormat = RES_FORMAT(FontLayout);

// Table of "well-known" resource formats, indexed by resource type.
#define MAX_SUPPORTED_TYPE RTYPE_FONT
const ResourceFormat *ResTypeLayout[MAX_SUPPORTED_TYPE + 1] = {
    NULL, // FIXME RTYPE_UNKNOWN (actually palette)
    NULL, // FIXME RTYPE_STRING
    NULL, // FIXME RTYPE_BITMAP
    &FontFormat // RTYPE_FONT
};

const ResourceFormat *ResLookUpFormat(Id id) {
    ResDesc2 *prd2 = RESDESC2(id);
    // If it's a compound resource, decode the ref table.
    if (prd2->flags & RDF_COMPOUND) {
	return FORMAT_REFTABLE;
    }
    // If it's a known resource type, return that format.
    if (prd2->type <= MAX_SUPPORTED_TYPE) {
	return ResTypeLayout[prd2->type];
    }
    // stub.
    return NULL;
}
