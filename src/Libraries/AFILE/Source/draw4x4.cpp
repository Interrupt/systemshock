//
// System Shock Enhanced Edition
//
// Copyright (C) 2015-2018 Night Dive Studios, LLC.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// DESCRIPTION:
//      4x4 Drawing routines
//		Reverse engineered by Alex Reimann Cunha Lima
//

//============================================================================
// Includes
#include "lg.h"
#include "2d.h"

// Holds bitstream information between invocations.
class BitstreamInfo
{
    uint8_t* ptr;
    uint32_t word; // working value
    int count;     // how many bits are available in word
public:
    explicit BitstreamInfo(uint8_t* p) : ptr(p), word(0), count(0) {}
    inline void reserve(int nbits) {
	while (count < nbits) {
	    word = (word << 8) | *ptr++;
	    count += 8;
	}
    }
    inline void skip(int nbits) {
	count -= nbits;
    }
    inline uint32_t peek(int nbits) {
	reserve(nbits);
	uint32_t const mask = ~(0xffffffff << nbits);
	return (word >> (count - nbits)) & mask;
    }
    inline uint32_t take(int nbits) {
	uint32_t const value = peek(nbits);
	skip(nbits);
	return value;
    }
};

static uchar* d4x4_hufftab;
static uchar* d4x4_colorset;

static uchar* Draw4x4_InternalBeta(uint32_t* xtab, int b, uchar* bits, int d, uchar* mask_stream);
static void Draw4x4_InternalAlpha(uint32_t* xtab, int b, BitstreamInfo& bitstream);

//============================================================================
// Functions

//
// Draw4x4Reset
//

extern "C" {

void Draw4x4Reset(uchar* colorset, uchar* hufftab)
{
    d4x4_hufftab = hufftab;
    d4x4_colorset = colorset;
}

//
// Draw4x4
//

void Draw4x4(uchar* p, int width, int height)
{
    uint32_t xtab[640 / 4];

    int cell_column;
    int row = grd_canvas->bm.row;
    cell_column = width / 4;
    BitstreamInfo bitstream(p+2);
    uchar* mask_stream = (p + *((ushort*)(p)));

    uchar* bits = grd_canvas->bm.bits;

    int row4 = row * 4;
    uint aligned_height = height & ~3;
    for (uint y = 0; y < aligned_height; y += 4) {
	Draw4x4_InternalAlpha(xtab, cell_column, bitstream);
	mask_stream = Draw4x4_InternalBeta(xtab, cell_column, bits, row, mask_stream);
	bits += row4;
    }
}

}

//
// Draw4x4_InternalAlpha
//
static void Draw4x4_InternalAlpha(uint32_t* xtab, int b, BitstreamInfo& bitstream)
{
    while (b > 0) {
	// Pull 12 bits from the bitstream and use it as an index into the
	// hufftable.
	auto const hindex = bitstream.peek(12);
	uint8_t* huffptr = &d4x4_hufftab[hindex * 3];
	uint32_t huffword = *((uint32_t*)huffptr) & 0x00FFFFFF;
	// Bits 20-23 are the count field.
	auto count = (huffword & 0xf00000) >> 20;
	if (count == 0) {
	    // A count of 0 is a long offset. It always advances the bitpointer
	    // by 12, since obviously it can't use the count to advance.
	    bitstream.skip(12);

	    while (count == 0) {
		// Use the entire (effectively 20-bit) huffword as an index into
		// the hufftable.
		huffptr = d4x4_hufftab + huffword * 3;
		// Pull 4 bits from the bitstream and add that to the huffindex.
		auto const offset = bitstream.peek(4);
		
		huffptr += offset * 3;
		// Check the next count: if it's still zero, go round again.
		huffword = *((uint32_t*)huffptr) & 0x00FFFFFF;
		count = (huffword & 0xf00000) >> 20;
		if (count == 0)
		{
		    // If we're going around again, eat the 4 bits of offset.
		    // Otherwise, let the new control word determine the count.
		    bitstream.skip(4);
		}
	    }
	}
	// Otherwise it is the count of bits to consume. This is not
	// necessarily the full 12 bits we just read; fields may overlap.
	bitstream.skip(count);

	b--;
	// Check the control word for special types.
	auto const type = (huffword & 0x000e0000) >> 17;
	switch (type)
	{
	case 5:
	{
	    // Type 5: skip. The next 5 bits from the bitstream form the count.
	    auto hcnt = bitstream.take(5);
	    // A count of 31 skips the rest of the row.
	    if (hcnt == 0x1f) {
		hcnt = b;
	    }
	    b -= hcnt;
	    *xtab++ = hcnt | 0x000a0000;
	    break;
	}
	case 6:
	case 7: // not used, but in the original exe has the same meaning as 6
	    // Type 6: repeat. Use the previous control word.
	    *xtab = xtab[-1];
	    ++xtab;
	    break;
	default:
	    // All other types are significant to the low-level decoder only.
	    *xtab++ = huffword;
	    break;
	}
    }
}

//
// Low-level decode routine. Takes the control-word table decoded above and
// the current mask stream pointer. Returns the updated mask_stream pointer.
//
static uint8_t* Draw4x4_InternalBeta(uint32_t* xtab, int framesize,
				     uint8_t* bits, int row,
				     uint8_t* mask_stream)
{
    uint8_t* tbits;    // temp copy of bits
    int i;           // generic loop index

    auto const end = bits + 4*framesize;
    while (bits < end) {
	// Read a control word from the decoded xtab and interpret it.
	// Bits 0-16 are the 'parameter' field.
	// Bits 17-19 are the 'type' field.
	// Bits 20-23 are only used by the first-stage decoder.
	auto const xtype = (*xtab >> 17) & 0x07;
	auto const xparam = *xtab & 0x01ffff;

        switch (xtype) {
        case 0:
	{
	    // Type 0 : direct colour. Bits 0-15 of the control word form a 2-
	    // pixel block to be replicated 8 times into the tile.
	    uint8_t ctab[2] = {
		uint8_t(xparam & 0xff),
		uint8_t((xparam & 0xff00) >> 8)
	    };
	    tbits = bits;
	    for (i = 0; i < 4; ++i) {
		tbits[0] = ctab[0];
		tbits[1] = ctab[1];
		tbits[2] = ctab[0];
		tbits[3] = ctab[1];
		tbits += row;
	    }
            break;
	}
        case 1:
	{
	    // Type 1 : 1-bit index. Read a 16-bit value from the mask stream.
	    // Each bit forms an index into a 2-byte colour table taken directly
	    // from bits 0-15 of the control word. Set each corresponding pixel
	    // in the tile accordingly, taking a zero value in the first colour
	    // table ONLY to be transparent.
	    uint8_t ctab[2] = {
		uint8_t(xparam & 0xff),
		uint8_t((xparam & 0xff00) >> 8)
	    };
	    auto mask = *(uint16_t*)mask_stream;
	    mask_stream += 2;
	    tbits = bits;
            if (ctab[0] != 0) {
		// No transparency, just blat to the tile.
                for (i = 0; i < 4; ++i) {
                    *tbits = ctab[mask&1];
                    tbits[1] = ctab[(mask>>1) & 1];
                    tbits[2] = ctab[(mask>>2) & 1];
                    tbits[3] = ctab[(mask>>3) & 1];
                    mask >>= 4;
                    tbits += row;
                }
            }
            else {
		// Have to take transparency into account.
		for (i = 0; i < 4; ++i) {
		    if (mask & 1) *tbits = ctab[mask&1];
		    if (mask & 2) tbits[1] = ctab[(mask>>1) & 1];
		    if (mask & 4) tbits[2] = ctab[(mask>>2) & 1];
		    if (mask & 8) tbits[3] = ctab[(mask>>3) & 1];
		    mask >>= 4;
		    tbits += row;
		}
            }
            break;
	}
        case 2:
	{
	    // Type 2: 2-bit index. Read a 32-bit value from the mask stream.
	    // The parameter field of the control word forms an index into the
	    // main colour table, taken to point to a 4-byte colour table. Set
	    // each tile pixel to the colour table indexed by the corresponding
	    // 2 bits of the mask word.
	    auto mask = *(uint32_t*)mask_stream;
	    mask_stream += 4;
            tbits = bits;
	    auto const ctab = d4x4_colorset + xparam;
            if (ctab[0] != 0) {
		// No transparency.
		for (i = 0; i < 4; ++i) {
		    tbits[0] = ctab[mask & 3];
                    tbits[1] = ctab[(mask>>2) & 3];
                    tbits[2] = ctab[(mask>>4) & 3];
                    tbits[3] = ctab[(mask>>6) & 3];
                    mask >>= 8;
                    tbits += row;
                }
                break;
            } else {
		for (i = 0; i < 4; ++i) {
		    if (mask & 0x03) tbits[0] = ctab[mask & 0x03];
		    if (mask & 0x0c) tbits[1] = ctab[(mask & 0x0c) >> 2];
		    if (mask & 0x30) tbits[2] = ctab[(mask & 0x30) >> 4];
		    if (mask & 0xc0) tbits[3] = ctab[(mask & 0xc0) >> 6];
		    mask >>= 8;
		    tbits += row;
		}
	    }
            break;
	}
        case 3:
	{
	    // Type 3: 3-bit index. As above, but read 48 bits from the mask
	    // stream and use each 3-bit field as an index into an 8-byte colour
	    // table.
	    // Note: rather than implement the ad-hoc schemes of the original
	    // code, I think it's more convenient and probably just as efficient
	    // to let the compiler deal with wide data types, even on a 32-bit
	    // platform.
	    auto mask = *(uint64_t*)mask_stream;
	    mask_stream += 6;
            tbits = bits;
	    auto const ctab = d4x4_colorset + xparam;
            if (ctab[0] != 0) {
		// No transparency.
		for (i = 0; i < 4; ++i) {
		    tbits[0] = ctab[mask & 7];
                    tbits[1] = ctab[(mask>>3) & 7];
                    tbits[2] = ctab[(mask>>6) & 7];
                    tbits[3] = ctab[(mask>>9) & 7];
                    mask >>= 12;
                    tbits += row;
                }
                break;
            } else {
		for (i = 0; i < 4; ++i) {
		    if (mask & 0x007) tbits[0] = ctab[mask & 0x007];
		    if (mask & 0x038) tbits[1] = ctab[(mask & 0x038) >> 3];
		    if (mask & 0x1c0) tbits[2] = ctab[(mask & 0x1c0) >> 6];
		    if (mask & 0xe00) tbits[3] = ctab[(mask & 0xe00) >> 9];
		    mask >>= 12;
		    tbits += row;
		}
	    }
            break;
	}
        case 4:
	{
	    // Type 4: 4-bit index. Mask word is 64 bits, colour table has 16
	    // entries.
	    auto mask = *(uint64_t*)mask_stream;
	    mask_stream += 8;
            tbits = bits;
	    auto const ctab = d4x4_colorset + xparam;
            if (ctab[0] != 0) {
		// No transparency.
		for (i = 0; i < 4; ++i) {
		    tbits[0] = ctab[mask & 0xf];
                    tbits[1] = ctab[(mask>>4) & 0xf];
                    tbits[2] = ctab[(mask>>8) & 0xf];
                    tbits[3] = ctab[(mask>>12) & 0xf];
                    mask >>= 16;
                    tbits += row;
                }
                break;
            } else {
		for (i = 0; i < 4; ++i) {
		    if (mask & 0x000f) tbits[0] = ctab[mask & 0x000f];
		    if (mask & 0x00f0) tbits[1] = ctab[(mask & 0x00f0) >> 4];
		    if (mask & 0x0f00) tbits[2] = ctab[(mask & 0x0f00) >> 8];
		    if (mask & 0xf000) tbits[3] = ctab[(mask & 0xf000) >> 12];
		    mask >>= 16;
		    tbits += row;
		}
	    }
            break;
	}
        case 5:
	    // Type 5: skip tiles horizontally.
            bits += xparam * 4;
            break;
        }
        ++xtab;
        bits += 4;
    }
    return mask_stream;
}
