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
//		HUFF.H	Huffman coding routines
//		Rex E. Bradford

/*
* $Header: r:/prj/lib/src/dstruct/RCS/huff.h 1.1 1994/08/22 17:13:06 rex Exp $
* $Log: huff.h $
 * Revision 1.1  1994/08/22  17:13:06  rex
 * Initial revision
 * 
*/

#ifndef __HUFF_H
#define __HUFF_H

//	This node is used when building a huffman coding tree.  You need to
//	allocate an array twice as big as the number of actual leaf nodes
//	that are to be coded.  Fill in the bottom half with leaf nodes
//	(token and freq) after clearing the entire array.  Then use HuffBuild()
//	to build the tree.

typedef struct _HuffNode {
	uint token;					// token value
	uint freq;						// frequency
	struct _HuffNode *parent;	// ptr to parent node
	struct _HuffNode *right;	// ptr to right node
	struct _HuffNode *left;		// ptr to left node
} HuffNode;

HuffNode *HuffBuild(HuffNode *htree, int numTerminals, int *pNumNodes);

//	You can walk a tree using HuffWalk().  HuffDump() uses the walker
//	with HuffPrintNode() to dump the tree.  HuffPrintNode() can be used
//	to print any node, and HuffWalk() may be used with your own routine
//	as well.

void HuffDump(HuffNode *htree, HuffNode *hroot, int numNodes);
void HuffWalk(HuffNode *htree, HuffNode *hroot,
	void (*func)(HuffNode *htree, HuffNode *hnode, uint code, int numBits));
void HuffPrintNode(HuffNode *htree, HuffNode *hnode, uint code, int numBits);

//	To speed up decoding, "flash tables" are used.  Normally, a huffman
//	stream must be examined a bit at a time, walking the tree till a
//	terminal node is hit.  A flash-tree looks up a number of bits at once,
//	into a table with redundant entries which encodes not only the lookup
//	token but also the number of bits used in that token.  When you make
//	a flash table, you can control the number of bits used to look up into
//	the primary table (default is 12-bit), and the number of bits used in
//	secondary tables, which are used when a given token is longer than the
//	primary table allows (default is 4-bit secondary tables, of which many
//	can be chained).  The overall token length in this implementation is
//	32 bits.
//
//	Because 4 bits are reserved for the "numbits" field of the flash table,
//	the actual active bits of a token encoded in a flash table is limited
//	to 28 bits.  Remember to include the 4 numbits bits when calculating
//	the width of your flash entries

#define HUFF_DEFAULT_BITS_PRI	12
#define HUFF_DEFAULT_BITS_SEC	4

int HuffMakeFlashTables(HuffNode *htree, HuffNode *hroot, uchar *pFlashTab,
	int pftLength, int tokSize, int bitsPri, int bitsSec);
void HuffPrintFlashTables(uchar *pFlashTab, uint length, int tokSize);
int HuffCompressFlashTables(uchar *pFlashTab, uint length, int tokSize,
	uint *pcBuff);

//	Flash tables must be decompressed to be used:

#ifdef __cplusplus
extern "C" {
#endif

void HuffExpandFlashTables(uchar *pFlashTab, uint lenTab, uint *pc,
	int tokSize);

#ifdef __cplusplus
}
#endif

//	Writing huffman-encoded data can be done using the following
//	routines and macros.  Make sure to use HuffEndOutput() to terminate
//	an output stream.  See huffde.h for decoding (reading) macros.

typedef struct {
	uchar *bits;			// ptr to huff-encoded bits
	int ibit;				// bit # within current byte, 0-7
} HuffPtr;

#define HuffResetWritePtr(ph,pbits) { \
	(ph)->bits=(pbits);	\
	(ph)->ibit=7;			\
	*((ph)->bits)=0;		\
}

void HuffEncode(HuffPtr *ph, uint code, int numBits);
void HuffEncodeNode(HuffPtr *ph, HuffNode *hnode);

#define HuffEndOutput(ph) ((ph)->bits++)

#define HuffStreamLen(ph,pbase) ((ph)->bits - pbase)

#endif

