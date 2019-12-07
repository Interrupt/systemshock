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
#if !defined(RESFORMAT_H)
#define RESFORMAT_H

// Types of resource field within a resfile.
typedef int ResFileFieldType;
#define RFFT_PAD    0  // not decoded, skip 'offset' bytes
#define RFFT_UINT8  1  // 8-bit integer
#define RFFT_UINT16 2  // 16-bit integer
#define RFFT_UINT32 3  // 32-bit integer
#define RFFT_INTPTR 4  // 32 bits on disc, 32 or 64 bits in memory.
#define RFFT_RAW    5  // raw data, copy 'offset' bytes or rest of resource if 0
#define RFFT_END    6  // mark end of table
#define RFFT_BIN_BASE 0x100

#define RFFT_BIN(x) (RFFT_BIN_BASE+(x))

// Describes the layout of a resource structure.
typedef struct {
    ResFileFieldType type;   // type of field
    size_t offset;           // offset in memory
} ResField;

typedef struct {
    size_t dsize;        // size of resource on disc
    size_t msize;        // size of resource in memory
    uint32_t flags;      // misc. info.
    ResField fields[];
} ResLayout;

// Indicates that multiple records exist within a resource, each of 'dsize'
// bytes, up to the resource size.
#define LAYOUT_FLAG_ARRAY            0x01
// Indicates that raw data (e.g. bitmap data) follows a header in the resource.
// The header is decoded according to the layout and the raw data is copied to
// immediately following it.
#define LAYOUT_FLAG_RAW_DATA_FOLLOWS 0x02

// User data for a resource decoding function.
typedef intptr_t UserDecodeData;
// Function to decode a resource loaded from disc. Takes raw data, size of raw
// data and user data. Returns decoded data. Default is to free decoded data
// using free() but the caller can supply a custom free function.
typedef void *(*ResDecodeFunc)(void*, size_t*, UserDecodeData);
// Encoder function. Same signature as decoder; takes cooked data and returns
// raw data for file, violating the laws of thermodynamics but allowing portable
// saving. Encoded data is always freed using free().
typedef void *(*ResEncodeFunc)(void*, size_t*, UserDecodeData);
// Function to free decoded data, if free() won't cut it.
typedef void (*ResFreeFunc)(void*);

// Decode a resource using a ResLayout. Prototyped as a decode function.
void *ResDecode(void *raw, size_t *size, UserDecodeData layout);
// Encode a resource using a ResLayout.
void *ResEncode(void *raw, size_t *size, UserDecodeData layout);

// Describes the format of a resource for serialisation and deserialisation to
// and from disc file.
typedef struct {
    ResDecodeFunc decoder; // deserialise data from disc.
    ResEncodeFunc encoder; // serialise data to disc.
    UserDecodeData data;   // aux data, typically a pointer to a layout struct.
    ResFreeFunc freer;     // free cooked (only) data.
} ResourceFormat;

// An empty ResourceFormat struct means no translation is needed.
extern const ResourceFormat RawFormat;
#define FORMAT_RAW (&RawFormat)

// Make a format out of a layout.
#define RES_FORMAT(layout) \
    { ResDecode, ResEncode, (UserDecodeData)&layout, NULL }

extern const ResourceFormat RefTableFormat;
#define FORMAT_REFTABLE (&RefTableFormat)

#endif // !defined(RESFORMAT_H)
