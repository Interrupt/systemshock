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

#include "2dres.h"
#include "grs.h" // grs_font

// Defines the layout of a FrameDesc within a resource file.
const ResLayout FrameDescLayout = {
  28, // on-disc size
  sizeof(FrameDesc), // in-memory size
  LAYOUT_FLAG_RAW_DATA_FOLLOWS, // flags
  {
    { RFFT_PAD, 4 }, // skip placeholder bits pointer
    { RFFT_UINT8,  offsetof(FrameDesc,bm.type) },
    { RFFT_UINT8,  offsetof(FrameDesc,bm.align) },
    { RFFT_UINT16, offsetof(FrameDesc,bm.flags) },
    { RFFT_UINT16, offsetof(FrameDesc,bm.w) },
    { RFFT_UINT16, offsetof(FrameDesc,bm.h) },
    { RFFT_UINT16, offsetof(FrameDesc,bm.row) },
    { RFFT_UINT8,  offsetof(FrameDesc,bm.wlog) },
    { RFFT_UINT8,  offsetof(FrameDesc,bm.hlog) },
    { RFFT_UINT16, offsetof(FrameDesc,updateArea.ul.x) },
    { RFFT_UINT16, offsetof(FrameDesc,updateArea.ul.y) },
    { RFFT_UINT16, offsetof(FrameDesc,updateArea.lr.x) },
    { RFFT_UINT16, offsetof(FrameDesc,updateArea.lr.y) },
    { RFFT_UINT32, offsetof(FrameDesc,pallOff) },
    { RFFT_RAW,    sizeof(FrameDesc) }, // raw bitmap data follows
    { RFFT_END, 0 }
  }
};

// Decoder function for frames: decodes using the layout in the normal way and
// then updates the bits pointer.
void *FrameDecode(void *raw, size_t *size, UserDecodeData layout) {
    FrameDesc *f = ResDecode(raw, size, layout);
    f->bm.bits = (uchar *)(f + 1);
    return f;
}
const ResourceFormat FrameDescFormat = {
    FrameDecode, ResEncode, (UserDecodeData)&FrameDescLayout, NULL };

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
#define MAX_SUPPORTED_TYPE RTYPE_MOVIE
const ResourceFormat *ResTypeLayout[MAX_SUPPORTED_TYPE + 1] = {
    NULL, // FIXME RTYPE_UNKNOWN (actually palette)
    &RawFormat,       // RTYPE_STRING (needs no translation)
    &FrameDescFormat, // RTYPE_BITMAP
    &FontFormat,      // RTYPE_FONT
    &RawFormat,       // FIXME RTYPE_ANIM
    NULL, // FIXME RTYPE_PALL
    NULL, // FIXME RTYPE_SHADTAB
    &RawFormat,       // RTYPE_VOC (not translated)
    NULL, // FIXME RTYPE_SHAPE
    NULL, // FIXME RTYPE_PICT
    NULL,             // RTYPE_B2EXTERN (not used)
    NULL,             // RTYPE_B2RELOC  (I think these are to do with the BABL
    NULL,             // RTYPE_B2CODE    conversation engine used by the
    NULL,             // RTYPE_B2HEADER  Underworld games. Not used by System
    NULL,             // RTYPE_B2RESRVD  Shock.)
    // OBJ3D types should have at least some translation, but looking at the
    // way the format is actually handled, I'm adopting the strategy of "back
    // away slowly, smiling but not showing your teeth" at the moment.
    &RawFormat, // FIXME RTYPE_OBJ3D
    NULL, // FIXME RTYPE_STENCIL
    &RawFormat, // FIXME RTYPE_MOVIE
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
    return NULL;
}

// Find the appropriate format for refs (compound resources).
const ResourceFormat *RefLookUpFormat(Id id) {
    ResDesc2 *prd2 = RESDESC2(id);
    // Shouldn't be used for non-compound resources.
    if (!(prd2->flags & RDF_COMPOUND)) {
	return NULL;
    }
    // If it's a known resource type, return that format.
    if (prd2->type <= MAX_SUPPORTED_TYPE) {
	return ResTypeLayout[prd2->type];
    }
    return NULL;
}
