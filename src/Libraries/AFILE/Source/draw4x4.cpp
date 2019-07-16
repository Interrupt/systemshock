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
#include <string.h>
#include "lg.h"
#include "2d.h"
#include "draw4x4.h"

static struct
{
    uchar* epsilon;
    int kappa;          // bit shift
} kapsi;

#define bitstream kapsi.epsilon
#define d4x4_kappa kapsi.kappa

static ushort grd_bank = 0; // FIXME: not needed in modern video modes anymore

static int d4x4_iota;
static uchar* d4x4_hufftab;
static uchar* d4x4_colorset;
static uint32_t d4x4_gamma;
static void* d4x4_beta;

static uchar** d4x4_eptr = &bitstream; // FIXME: should be ptr to something
static uint32_t* d4x4_gptr = &d4x4_gamma; // FIXME: should be ptr to something

static uchar d4x4_omega[20];  // should be int*

// Reimann: FIXME: hack to prevent using SVGA banks. Should alter the code instead of using this hack
#define grd_mode 0

//
// Assembly instructions emulation
//
#define asm_shr(a, b) a = (uint)a >> b
#define movsd() *(int*)edi = *(int*)esi; esi += 4; edi += 4

static uchar* Draw4x4_InternalBeta(int* xtab, int b, uchar* bits, int d, uchar* mask_stream);
static void Draw4x4_InternalAlpha(int* xtab, int b);

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
    if (grd_mode != 0)
    {
        if (grd_canvas->bm.type == BMT_DEVICE)
        {
            if (d4x4_beta)
            {
                memset(d4x4_beta, 0, 0x3200);
            }
        }
    }    
}

//
// Draw4x4
//

void Draw4x4(uchar* p, int width, int height)
{
    int xtab[640 / 4];

    int cell_column;
    int esp_2AC = 0;
    int row = grd_canvas->bm.row;
    cell_column = width / 4;
    bitstream = p + 2;
    d4x4_gamma = 0;                                     // mov     dword [d4x4_gamma], edx
    d4x4_kappa = 0;                                     // mov     dword [d4x4_kappa], edx
    uchar* mask_stream = (p + *((ushort*)(p)));         // mov     dword [esp+0x290], ecx

    uchar* bits = grd_canvas->bm.bits;

    int row4 = row * 4;                                 // mov     dword [esp+0x29C], eax
    uint aligned_height = height & ~3;                  // mov     dword [esp+0x2A8], eax
    int esp_2B0 = esp_2AC * 4;                          // mov     dword [esp+0x2B0], eax
    uint y = 0;                                         // mov     dword [esp+0x2A0], ecx
    if (aligned_height)
    {
        do
        {
            Draw4x4_InternalAlpha(xtab, cell_column);   // call    near Draw4x4_InternalAlpha_
            mask_stream = Draw4x4_InternalBeta(xtab, cell_column, bits, row, mask_stream);
                                                        // call    near Draw4x4_InternalBeta_
            bits += row4;                               // add     esi, dword [esp+0x29C]
            y += 4;
        } while (y < aligned_height);
    }
    return;                                             // ret
}

}

//
// Draw4x4_InternalGamma
//

static uchar* Draw4x4_InternalGamma(uchar* peax)
{
    uchar* esi;
    int ecx;
    int edx;
    int ebx;
    int eax;
    
    ebx = d4x4_gamma;                                   // mov     ebx, dword [_d4x4_gamma]
    esi = d4x4_hufftab;                                 // mov     esi, dword [_d4x4_hufftab]
    d4x4_kappa -= 0x0C;                                 // sub     dword [_d4x4_kappa], byte 0x0C
L0026AA24:
    
    eax = *((int*)peax);                                // mov     eax, dword [eax]
    eax &= 0x00FFFFFF;                                  // and     eax, dword 0x00FFFFFF
    eax = eax * 3;                                      // lea     eax, [eax+eax*2]
    ecx = d4x4_kappa;                                   // mov     ecx, dword [_d4x4_kappa]
    peax = esi + eax;                                   // add     eax, esi
                                                        // cmp     ecx, byte 0x04
    if (ecx < 4)                                        // jge     short L0026AA61
    {
        uchar* pedx;
        int edi;
        
        pedx = bitstream;                               // mov     edx, dword [_d4x4_epsilon]
        edi = d4x4_kappa;                               // mov     edi, dword [_d4x4_kappa]
        ebx <<= 8;                                      // shl     ebx, 0x08
        ecx = *pedx;
        pedx++;                                         // inc     edx
        //ecx = 0;                                      // xor     ecx, ecx
        edi += 8;                                       // add     edi, byte 0x08
        //cl = (byte) edx[-1];                          // mov     cl, byte [edx-0x1]
        bitstream = pedx;                               // mov     dword [_d4x4_epsilon], edx
        d4x4_kappa = edi;                               // mov     dword [_d4x4_kappa], edi
        ebx |= ecx;                                     // or      ebx, ecx
    }
    ecx = d4x4_kappa;                                   // mov     ecx, dword [_d4x4_kappa]
    edx = ebx;                                          // mov     edx, ebx
    ecx -= 4;                                           // sub     ecx, byte 0x04
    edx = ((uint)edx) >> ecx;                           // shr     edx, cl
    ecx = edx;                                          // mov     ecx, edx
    ecx &= 0x0F;                                        // and     ecx, byte 0x0F
    edx = ecx * 3;                                      // lea     edx, [ecx+ecx*2]
    peax += edx;                                        // add     eax, edx
    edx = *((int*)peax);                                // mov     edx, dword [eax]
    edx = ((uint)edx) >> 0x14;                          // shr     edx, 0x14
                                                        // test    dl, byte 0x0F
    if ((edx & 0x0F) == 0)                              // jnz     short L0026AA8B
    {
        d4x4_kappa -= 4;                                // sub     dword [_d4x4_kappa], byte 0x04
        goto L0026AA24;                                 // jmp     short L0026AA24
    }
    d4x4_hufftab = esi;                                 // mov     dword [_d4x4_hufftab], esi
    d4x4_iota = ecx;                                    // mov     dword [_d4x4_iota], ecx
    d4x4_gamma = ebx;                                   // mov     dword [_d4x4_gamma], ebx
    return peax;
}

//
// Draw4x4_InternalAlpha
//
// xtab passed in eax
// b passed in edx
static void Draw4x4_InternalAlpha(int* xtab, int b)
{
    uchar *esp_00;   // local copy of kapsi.epsilon
    int esp_04;      // local copy of kapsi.kappa
    uint32_t gamma;  // register eax
    uint32_t gtmp;   // extracted from gamma
    uchar *huffptr;  // points into hufftable
    uint32_t hcnt;   // extracted from huff, subtracted from b
    uint32_t htmp;   // extracted from huff
    uint32_t huffhi; // bits 20-24 of a huffword
    //pushra();
    //push(ebx);                                        // push    ebx
    //push(ecx);                                        // push    ecx
    //push(esi);                                        // push    esi
    //push(edi);                                        // push    edi
    //push(ebp);                                        // push    ebp
    //esp -= 0x08;                                      // sub     esp, byte 0x08
                                                        // mov     ebx, eax
                                                        // mov     eax, dword [_d4x4_gptr]
                                                        // mov     edi, esp
                                                        // mov     esi, dword [_d4x4_eptr]
    gamma = *d4x4_gptr;                                 // mov     eax, dword [eax]
    // Note some jiggery-pokery with the movsd instructions here.
    // d4x4_eptr actually points to bitstream, i.e. kapsi.epsilon.
    // So we're just copying the 2 members of the kapsi struct
    // onto the local stack.
    esp_00 = kapsi.epsilon;                             // movsd
    esp_04 = kapsi.kappa;                               // movsd
L0026AABF:
                                                        // test    edx, edx
    if (b <= 0) goto L0026ACE6;                       // jle     near L0026ACE6
                                                        // cmp     edx, byte 0x01
    if (b <= 0x01) goto L0026AAD5;                    // jle     short L0026AAD5
                                                        // cmp     dword [esp+0x4], byte 0x18
    if (esp_04 > 0x18) goto L0026AB1B;      // jg      short L0026AB1B
    goto L0026AB01;                                     // jmp     short L0026AB01
L0026AAD5:
                                                        // mov     edi, dword [esp+0x4]
                                                        // cmp     edi, byte 0x0C
    if (esp_04 >= 0x0C) goto L0026AB7A;                 // jge     near L0026AB7A
                                                        // mov     ecx, dword [esp]
                                                        // lea     ebp, [ecx+0x1]
                                                        // mov     dword [esp], ebp
                                                        // mov     cl, byte [ecx]
                                                        // and     ecx, dword 0x000000FF
                                                        // shl     eax, 0x08
    gamma = (gamma << 8) | *esp_00++;                   // or      eax, ecx
                                                        // lea     ecx, [edi+0x8]
    esp_04 += 8;                                        // mov     dword [esp+0x4], ecx
    goto L0026AAD5;                                     // jmp     short L0026AAD5
L0026AB01:
                                                        // mov     edi, esp
                                                        // mov     ecx, dword [edi+0x4]
                                                        // mov     esi, dword [edi]
    do { // L0026AB08:
                                                        // shl     eax, 0x08
                                                        // mov     al, byte [esi]
        gamma = (gamma << 8) | *esp_00++;               // inc     esi
        esp_04 += 0x08;                                 // add     ecx, byte 0x08
                                                        // cmp     ecx, byte 0x19
    } while (esp_04 < 0x19);                            // jl      short L0026AB08
                                                        // mov     dword [edi+0x4], ecx
                                                           // mov     dword [edi], esi
L0026AB1B:
                                                        // mov     ecx, dword [esp+0x4]
                                                        // mov     edi, eax
                                                        // sub     ecx, byte 0x0C
                                                        // shr     edi, cl
    gtmp = (gamma >> (esp_04-12)) & 0x00000FFF;         // and     edi, dword 0x00000FFF
                                                        // lea     ecx, [edi*4]
                                                        // mov     esi, dword [_d4x4_hufftab]
                                                        // sub     ecx, edi
    huffptr = &d4x4_hufftab[gtmp*3];                    // add     ecx, esi
    htmp = *((uint32_t*)huffptr);                       // mov     esi, dword [ecx]
                                                        // mov     edi, esi
                                                        // shr     edi, 0x14
    huffhi = (htmp >> 20) & 0x0F;                       // and     edi, byte 0x0F
                                                        // test    edi, edi
    if (huffhi == 0) goto L0026AC00;                    // jz      near L0026AC00
                                                        // mov     ebp, dword [esp+0x4]
    esp_04 -= huffhi;                                   // sub     ebp, edi
                                                        // mov     edi, esi
    xtab++;                                             // add     ebx, byte 0x04
    htmp &= 0x00FFFFFF;                                 // and     edi, dword 0x00FFFFFF
    b -= 1;                                             // dec     edx
                                                        // mov     ecx, edi
                                                        // mov     dword [esp+0x4], ebp
                                                        // and     ecx, dword 0x000E0000
    xtab[-1] = htmp;                                    // mov     dword [ebx-0x4], edi
                                                        // cmp     ecx, dword 0x000A0000
    if ((htmp & 0x000e0000) >= 0x000a0000) goto L0026ABEA; // jnc     near L0026ABEA
L0026AB7A:
                                                        // mov     ecx, dword [esp+0x4]
                                                        // mov     esi, eax
                                                        // sub     ecx, byte 0x0C
                                                        // shr     esi, cl
                                                        // mov     ecx, esi
    gtmp = (gamma >> (esp_04 - 12)) & 0x00000fff;       // and     ecx, dword 0x00000FFF
                                                        // lea     edi, [ecx*4]
                                                        // sub     edi, ecx
                                                        // mov     ecx, dword [_d4x4_hufftab]
    huffptr = &d4x4_hufftab[gtmp*3];                    // add     ecx, edi
    htmp = *((uint32_t*)huffptr);                       // mov     edi, dword [ecx]
                                                        // mov     esi, edi
                                                        // shr     esi, 0x14
    huffhi = (htmp >> 20) & 0x0f;                       // and     esi, byte 0x0F
                                                        // test    esi, esi
    if (huffhi == 0) goto L0026AC00;                    // jz      short L0026AC00
                                                        // mov     ecx, dword [esp+0x4]
    htmp &= 0x00FFFFFF;                                 // and     edi, dword 0x00FFFFFF
                                                        // sub     ecx, esi
    xtab++;                                             // add     ebx, byte 0x04
    esp_04 -= huffhi;                                   // mov     dword [esp+0x4], ecx
                                                        // mov     ecx, edi
    b--;                                                // dec     edx
                                                        // and     ecx, dword 0x000E0000
    xtab[-1] = htmp;                                    // mov     dword [ebx-0x4], edi
                                                        // cmp     ecx, dword 0x000A0000
    if ((htmp & 0x000e0000) >= 0x000a0000) goto L0026ABEA;// jnc     short L0026ABEA
                                                        // test    edx, edx
    if (b == 0) goto L0026ABEA;                         // jz      short L0026ABEA
                                                        // cmp     dword [esp+0x4], byte 0x0C
    if (esp_04 >= 0x0C) goto L0026AB7A;                 // jge     short L0026AB7A
                                                        // cmp     edx, byte 0x01
    if (b > 0x01) goto L0026AB01;                       // jg      near L0026AB01
    goto L0026AAD5;                                     // jmp     near L0026AAD5
L0026ABEA:
                                                        // mov     ecx, edi
                                                        // and     ecx, dword 0x000E0000
                                                        // cmp     ecx, dword 0x000A0000
    if ((htmp & 0x000e0000) < 0x000a0000) goto L0026ACE6;// jc      near L0026ACE6
    goto L0026AC6A;                                     // jmp     short L0026AC6A
L0026AC00:
                                                        // mov     esi, dword [_d4x4_gptr]
                                                        // mov     edi, dword [_d4x4_eptr]
    *d4x4_gptr = gamma;                                 // mov     dword [esi], eax
                                                        // mov     esi, esp
                                                        // mov     eax, ecx
    kapsi.epsilon = esp_00;                             // movsd
    kapsi.kappa = esp_04;                               // movsd
                                                        // call    near Draw4x4_InternalGamma_
    huffptr = Draw4x4_InternalGamma(huffptr);           // mov     ecx, eax
                                                        // mov     eax, dword [_d4x4_gptr]
                                                        // mov     edi, esp
                                                        // mov     esi, dword [_d4x4_eptr]
    gamma = *d4x4_gptr;                                 // mov     eax, dword [eax]
    esp_00 = kapsi.epsilon;                             // movsd
    esp_04 = kapsi.kappa;                               // movsd
                                                        // mov     esi, dword [ecx]
                                                        // shr     esi, 0x14
                                                        // mov     edi, dword [esp+0x4]
    huffhi = (*((uint32_t*)huffptr) >> 20) & 0x0f;      // and     esi, byte 0x0F
                                                        // sub     edi, esi
    esp_04 -= huffhi;                                   // mov     dword [esp+0x4], edi
                                                        // mov     edi, dword [ecx]
    htmp = *((uint32_t*)huffptr) & 0x00FFFFFF;          // and     edi, dword 0x00FFFFFF
    xtab++;                                             // add     ebx, byte 0x04
                                                        // mov     ecx, edi
    b--;                                                // dec     edx
                                                        // and     ecx, dword 0x000E0000
    xtab[-1] = htmp;                                    // mov     dword [ebx-0x4], edi
                                                        // cmp     ecx, dword 0x000A0000
    if ((htmp & 0x000e0000) >= 0x000a0000) goto L0026AC6A;// jnc     short L0026AC6A
                                                        // test    edx, edx
    if (b == 0) goto L0026ACE6;                         // jz      near L0026ACE6
    goto L0026AABF;                                     // jmp     near L0026AABF
L0026AC6A:
    htmp &= 0x000E0000;                                 // and     edi, dword 0x000E0000
    xtab--;                                             // sub     ebx, byte 0x04
                                                        // cmp     edi, dword 0x000A0000
    if (htmp == 0x000a0000) {                           // jnz     short L0026ACD8
                                                        // mov     ebp, dword [esp+0x4]
                                                        // cmp     ebp, byte 0x05
        if (esp_04 < 5) {                               // jge     short L0026ACA1
                                                        // mov     ecx, dword [esp]
                                                        // lea     esi, [ecx+0x1]
                                                        // mov     dword [esp], esi
                                                        // mov     cl, byte [ecx]
                                                        // and     ecx, dword 0x000000FF
                                                        // lea     edi, [ebp+0x8]
                                                        // shl     eax, 0x08
            esp_04 += 8;                                // mov     dword [esp+0x4], edi
            gamma = (gamma < 8) | *esp_00++;            // or      eax, ecx
        } // L0026ACA1:
                                                        // mov     ecx, dword [esp+0x4]
                                                        // mov     esi, eax
                                                        // sub     ecx, byte 0x05
                                                        // shr     esi, cl
                                                        // mov     ebp, dword [esp+0x4]
                                                        // mov     ecx, esi
                                                        // sub     ebp, byte 0x05
        hcnt = (gamma >> (esp_04-5)) & 0x1f;            // and     ecx, byte 0x1F
        esp_04 -= 5;                                    // mov     dword [esp+0x4], ebp
                                                        // cmp     ecx, byte 0x1F
        if (hcnt == 0x1f) {                             // jnz     short L0026ACC3
            hcnt = b;                                   // mov     ecx, edx
        } // L0026ACC3:
                                                        // mov     esi, ecx
        xtab++;                                         // add     ebx, byte 0x04
                                                        // or      esi, dword 0x000A0000
        b -= hcnt;                                      // sub     edx, ecx
        xtab[-1] = hcnt | 0x000a0000;                   // mov     dword [ebx-0x4], esi
    }                                                   // jmp     near L0026AABF
    else { // L0026ACD8:
        xtab++;                                         // add     ebx, byte 0x04
                                                        // mov     ecx, dword [ebx-0x8]
        xtab[-1] = xtab[-2];                            // mov     dword [ebx-0x4], ecx
    }
    goto L0026AABF;                                     // jmp     near L0026AABF
L0026ACE6:
                                                        // mov     edx, dword [_d4x4_gptr]
                                                        // mov     esi, esp
                                                        // mov     edi, dword [_d4x4_eptr]
    *d4x4_gptr = gamma;                                 // mov     dword [edx], eax
    kapsi.epsilon = esp_00;                             // movsd
    kapsi.kappa = esp_04;                               // movsd
    //esp += 0x08;                                      // add     esp, byte 0x08
    //ebp = pop();                                      // pop     ebp
    //edi = pop();                                      // pop     edi
    //esi = pop();                                      // pop     esi
    //ecx = pop();                                      // pop     ecx
    //ebx = pop();                                      // pop     ebx
    //popra();
    //return;                                           // ret
}

//
// Draw4x4_InternalBeta
//
// Original machine code function took:
//  xtab in eax,
//  b in edx,
//  bits in ebx,
//  d in ecx,
//  mask_stream on stack
static uchar* Draw4x4_InternalBeta(int* xtab, int b, uchar* bits, int d, uchar* mask_stream)
{
    char cl0, dl0, bl0;
    char al0, ah0;

    uchar *esp_00;
    uint32_t esp_04;
    uchar *esp_14;
    int esp_20;
    uchar *esp_24; // points into d4x4_colorset
    uchar *cbits;    // points into d4x4_colorset
    int esp_28; // loop variable
    uchar *esp_2c;
    uint32_t mask32; // temp uint32 pulled from mask_stream
    uint32_t m32a;   // additional mask data
    uchar *tbits;    // temp copy of bits
    uint32_t tab32;  // uint32 derived from xtab
    uint16_t tab16;  // uint16 derived from xtab
    uchar *xbits;    // byte array derived from xtab
    int i;           // generic loop index
    //pushra();
    //push(esi);                                        // push    esi
    //push(edi);                                        // push    edi
    //push(ebp);                                        // push    ebp
    //esp -= 0x38;                                      // sub     esp, byte 0x38
    uchar *ebp = mask_stream;                           // mov     ebp, dword [esp+0x48]
    // esp+0x18 = spill space for xtab                  // mov     dword [esp+0x18], eax
    // esp+0x1c = spill space for bits                  // mov     dword [esp+0x1C], ebx
    // esp+0x30 = spill space for d                     // mov     dword [esp+0x30], ecx
                                                        // lea     eax, [ecx-0x4]
                                                        // shl     edx, 0x02
    esp_20 = d - 4;                                     // mov     dword [esp+0x20], eax
                                                        // lea     eax, [ebx+edx]
    uchar *esp_10 = bits + 4*b;                         // mov     dword [esp+0x10], eax
                                                        // cmp     ebx, eax
                                                        // jnc     near L0026B221
                                                        // lea     eax, [ecx*4]
                                                        // sub     eax, ecx
    int esp_0c = d * 3;                                 // mov     dword [esp+0xC], eax
                                                        // lea     eax, [ecx+ecx]
    int esp_08 = d * 2;                                 // mov     dword [esp+0x8], eax
    while (bits < esp_10) { //    outer_loop:
                                                        // mov     eax, dword [esp+0x18]
                                                        // mov     eax, dword [eax]
                                                        // and     eax, dword 0x0001FFFF
        esp_04 = *xtab & 0x0001ffff;                    // mov     dword [esp+0x4], eax
                                                        // mov     eax, dword [esp+0x18]
                                                        // mov     eax, dword [eax]
                                                        // shr     eax, 0x11
                                                        // and     eax, byte 0x07
                                                        // cmp     eax, byte 0x05
                                                        // ja      near L0026B1FF
                                                        // mov     edx, dword [esp+0x1C]
                                                        // mov     edi, dword [esp+0x8]
                                                        // add     edx, edi
        esp_14 = bits + esp_08;                         // mov     dword [esp+0x14], edx
                                                        // lea     edx, [ebp+0x4]
        esp_00 = ebp + 4;                               // mov     dword [esp], edx
    
        switch ((((unsigned int)*xtab) >> 0x11) & 0x07) // jmp     dword [cs:eax*4+L0026AD04]
        {
        case 0: //   L0026AD9F:
	                                                // mov     eax, dword [esp+0x4]
                                                        // mov     esi, dword [esp+0x4]
                                                        // shl     eax, 0x10
                                                        // mov     edx, dword [esp+0x1C]
            tab32 = (esp_04 << 0x10) | esp_04;          // or      eax, esi
                                                        // mov     edi, dword [esp+0x30]
            *((uint32_t*) bits) = tab32;                // mov     dword [edx], eax
                                                        // add     edx, edi
            *((uint32_t*)(bits+d)) = tab32;             // mov     dword [edx], eax
                                                        // mov     edx, dword [esp+0x14]
                                                        // mov     ebx, dword [esp+0xC]
            *((uint32_t*)esp_14) = tab32;               // mov     dword [edx], eax
                                                        // mov     edx, dword [esp+0x1C]
                                                        // add     edx, ebx
            *((uint32_t*)(bits + esp_0c)) = tab32;      // mov     dword [edx], eax
            break;                                      // jmp     near L0026B1FF
        case 1: //    L0026ADD1:
            tbits = bits;                               // mov     eax, dword [esp+0x1C]
            xbits = (uchar*)&esp_04;                    // lea     esi, [esp+0x4]
            ebp += 0x02;                                // add     ebp, byte 0x02
                                                        // xor     edx, edx
                                                        // mov     ch, byte [esp+0x4]
            mask32 = *((ushort*)(ebp-0x2));             // mov     dx, word [ebp-0x2]
                                                        // test    ch, ch
            if (esp_04 & 0xff != 0) {                   // jz      short L0026AE34
                                                        // mov     ebx, dword [esp+0x30]
                for (i = 0; i < 4; ++i) {               // xor     edi, edi
//L0026ADF0:
                                                        // mov     ecx, edx
                                                        // and     ecx, byte 0x01
		                                        // mov     cl, byte [ecx+esi]
                    *tbits = xbits[mask32&1];           // mov     byte [eax], cl
                                                        // mov     ecx, edx
                                                        // shr     ecx, 0x00000001
                                                        // and     ecx, byte 0x01
                                                        // mov     cl, byte [ecx+esi]
                    tbits[1] = xbits[(mask32>>1) & 1];  // mov     byte [eax+0x1], cl
                                                        // mov     ecx, edx
                                                        // shr     ecx, 0x02
                                                        // and     ecx, byte 0x01
                                                        // mov     cl, byte [ecx+esi]
                    tbits[2] = xbits[(mask32>>2) & 1];  // mov     byte [eax+0x2], cl
                                                        // mov     ecx, edx
                                                        // shr     ecx, 0x03
                                                        // and     ecx, byte 0x01
                                                        // inc     edi
                                                        // mov     cl, byte [ecx+esi]
                                                        // shr     edx, 0x04
                    tbits[3] = xbits[(mask32>>3) & 1];  // mov     byte [eax+0x3], cl
                    mask32 >>= 4;
                    tbits += d;                         // add     eax, ebx
                                                        // cmp     edi, byte 0x04
                }                                       // jge     near L0026B1FF
            }                                           // jmp     short L0026ADF0
            else { //    L0026AE34:
                                                        // xor     ecx, ecx
                tab16 = (esp_04 >> 8) & 0xff;           // mov     cl, byte [esp+0x5]
                                                        // mov     ebx, ecx
                                                        // mov     edi, dword 0x00000004
                                                        // shl     ebx, 0x08
                                                        // mov     esi, dword [esp+0x30]
                tab16 |= tab16 << 8;                    // or      ecx, ebx
                for (i = 4; i > 0; --i) { // L0026AE4A:
		                                        // test    dl, byte 0x03
                    if (mask32 & 0x03) {                // jz      short L0026AE65
                                                        // test    dl, byte 0x01
                        if (mask32 & 0x01) {            // jz      short L0026AE62
                                                        // test    dl, byte 0x02
	                    if (mask32 & 0x02) {        // jz      short L0026AE5E
                                *((ushort*)tbits) = tab16; // mov     word [eax], cx
                            }                           // jmp     short L0026AE65
                            else { //    L0026AE5E:
                                *tbits = tab16 & 0xff;  // mov     byte [eax], cl
                            }
                        }                               // jmp     short L0026AE65
                        else { //    L0026AE62:
                            tbits[1] = tab16 & 0xff;    // mov     byte [eax+0x1], cl
                        }
                    } //    L0026AE65:
                                                        // test    dl, byte 0x0C
                    if (mask32 & 0x0C) {                // jz      short L0026AE82
                                                        // test    dl, byte 0x04
                        if (mask32 & 0x04) {            // jz      short L0026AE7F
                                                        // test    dl, byte 0x08
                            if (mask32 & 0x08) {        // jz      short L0026AE7A
                                *((ushort*)(tbits+0x2)) = tab16; // mov     word [eax+0x2], cx
                            }                           // jmp     short L0026AE82
			    else { //    L0026AE7A:
                                tbits[2] = tab16 & 0xff; // mov     byte [eax+0x2], cl
                            }
                        }                               // jmp     short L0026AE82
                        else { //    L0026AE7F:
                            tbits[3] = tab16 & 0xff;    // mov     byte [eax+0x3], cl
                        }
                    } //    L0026AE82:
                    mask32 >>= 4;                       // shr     edx, 0x04
                                                        // dec     edi
                    tbits += d;                         // add     eax, esi
                                                        // test    edi, edi
                                                        // jle     near L0026B1FF
                }                                       // jmp     short L0026AE4A
            }
            break;
        case 2: //    L0026AE92:
                                                        // mov     eax, dword [ebp]
            mask32 = *(uint32_t*)ebp;                   // mov     dword [esp+0x34], eax
            ebp = esp_00;                               // mov     ebp, edx
                                                        // mov     edx, dword [esp+0x4]
                                                        // mov     eax, dword [_d4x4_colorset]
                                                        // add     edx, eax
            tbits = bits;                               // mov     edi, dword [esp+0x1C]
                                                        // mov     cl, byte [edx]
            esp_24 = d4x4_colorset + esp_04;            // mov     dword [esp+0x24], edx
                                                        // test    cl, cl
            if (*esp_24) {                              // jz      near L0026AF6A
                                                        // mov     dword [esp+0x28], dword 0x00000004
                for (esp_28 = 4; esp_28 > 0; esp_28 -= 2) { //    L0026AEC0:
                                                        // mov     esi, dword [esp+0x24]
                                                        // mov     edx, dword [esp+0x34]
                                                        // mov     ecx, edi
                                                        // mov     eax, edx
                                                        // and     eax, byte 0x03
                    tbits[0] = esp_24[mask32 & 3];      // mov     bl, byte [esi+eax]
                                                        // mov     eax, edx
                                                        // shr     eax, 0x02
                                                        // and     eax, byte 0x03
                                                        // mov     bh, byte [esi+eax]
                    tbits[1] = esp_24[(mask32>>2) & 3]; // mov     word [ecx], bx
                                                        // mov     eax, edx
                                                        // shr     eax, 0x04
                                                        // and     eax, byte 0x03
                    tbits[2] = esp_24[(mask32>>4) & 3]; // mov     bl, byte [esi+eax]
                                                        // mov     eax, edx
                                                        // shr     eax, 0x06
                                                        // and     eax, byte 0x03
                                                        // mov     bh, byte [esi+eax]
                    tbits[3] = esp_24[(mask32>>6) & 3]; // mov     word [ecx+0x2], bx
                                                        // mov     ecx, edx
                                                        // mov     esi, dword [esp+0x30]
                                                        // shr     ecx, 0x08
                    tbits += d;                         // add     edi, esi
                    mask32 >>= 8;                       // mov     dword [esp+0x34], ecx
                                                        // mov     esi, dword [esp+0x24]
                                                        // mov     edx, dword [esp+0x34]
                                                        // mov     ecx, edi
                                                        // mov     eax, edx
                                                        // and     eax, byte 0x03    
                    tbits[0] = esp_24[mask32 & 3];      // mov     bl, byte [esi+eax]
                                                        // mov     eax, edx
                                                        // shr     eax, 0x02
                                                        // and     eax, byte 0x03
                                                        // mov     bh, byte [esi+eax]
                    tbits[1] = esp_24[(mask32>>2) & 3]; // mov     word [ecx], bx
                                                        // mov     eax, edx
                                                        // shr     eax, 0x04
                                                        // and     eax, byte 0x03
                    tbits[2] = esp_24[(mask32>>4) & 3]; // mov     bl, byte [esi+eax]
                                                        // mov     eax, edx
                                                        // shr     eax, 0x06
                                                        // and     eax, byte 0x03
                                                        // mov     bh, byte [esi+eax]
                    tbits[3] = esp_24[(mask32>>6) & 3]; // mov     word [ecx+0x2], bx
                                                        // mov     ebx, dword [esp+0x28]
                                                        // mov     eax, edx
                                                        // mov     edx, dword [esp+0x30]
                                                        // shr     eax, 0x08
                                                        // sub     ebx, byte 0x02
                    mask32 >>= 8;                       // mov     dword [esp+0x34], eax
                                                        // mov     dword [esp+0x28], ebx
                    tbits += d;                         // add     edi, edx
                                                        // test    ebx, ebx
                }                                   // jle     near L0026B1FF
                break;                              // jmp     near L0026AEC0
            } //    L0026AF6A:
	    // Local variable esp+0x34 is only used by this case. This code
	    // path doesn't actually spill it back to memory in the original
	    // machine code, but it doesn't hurt to do so.
                                                    // mov     ebx, dword [esp+0x34]
                                                        // mov     esi, edx
                                                        // mov     edx, dword [esp+0x30]
            for (ah0 = 4; ah0 != 0; --ah0) {            // mov     ah, byte 0x04
//simple_loop:
                                                        // mov     ecx, ebx
                                                        // and     ecx, byte 0x03
                                                        // test    ecx, ecx
                if (mask32 & 0x03) {                    // jz      short L0026AF82
                                                        // mov     al, byte [esi+ecx]
                    tbits[0] = esp_00[mask32 & 0x03];   // mov     byte [edi], al
                } //    L0026AF82:
                                                        // mov     ecx, ebx
		                                        // and     ecx, byte 0x0C
                                                        // test    ecx, ecx
                if (mask32 & 0x0c) {                    // jz      short L0026AF92
                                                        // shr     ecx, 0x02
                                                        // mov     al, byte [esi+ecx]
                    tbits[1] = esp_00[(mask32 & 0x0c) >> 2]; // mov     byte [edi+0x1], al
                } //    L0026AF92:
                                                        // mov     ecx, ebx
                                                        // and     ecx, byte 0x30
                                                        // test    ecx, ecx
                if (mask32 & 0x30) {                    // jz      short L0026AFA2
                                                        // shr     ecx, 0x04
                                                        // mov     al, byte [esi+ecx]
                    tbits[2] = esp_00[(mask32 & 0x30) >> 4]; // mov     byte [edi+0x2], al
                } //    L0026AFA2:
                                                        // mov     ecx, ebx
                                                        // and     ecx, dword 0x000000C0
                                                        // test    ecx, ecx
                if (mask32 & 0xc0) {                    // jz      short L0026AFB5
                                                        // shr     ecx, 0x06
                                                        // mov     al, byte [esi+ecx]
                    tbits[3] = esp_00[(mask32 & 0xc0) >> 6]; // mov     byte [edi+0x3], al
                } //    L0026AFB5:
                tbits += d;                             // add     edi, edx
                mask32 >>= 8;                           // shr     ebx, 0x08
                                                        // dec     ah
            }                                       // jnz     short simple_loop
            break;                                  // jmp     near L0026B1FF
        case 3: //    L0026AFC3:
                                                        // mov     ebx, dword [esp+0x4]
                                                        // mov     esi, dword [_d4x4_colorset]
            cbits = d4x4_colorset + esp_04;             // add     esi, ebx
            tbits = bits;                               // mov     edx, dword [esp+0x1C]
            mask32 = *((uint32_t*)ebp);                 // mov     eax, dword [ebp]
                                                        // mov     bh, byte [esi]
            ebp = esp_00;                               // mov     ebp, dword [esp]
                                                        // test    bh, bh
            if (*cbits) {                               // jz      short L0026B03D
                                                        // mov     ebx, dword [esp+0x30]
                for (i = 0; i < 4; ++i) {               // xor     edi, edi
//L0026AFE5:
                                                        // mov     ecx, eax
                                                        // and     ecx, byte 0x07
                                                        // mov     cl, byte [ecx+esi]
                    *tbits = cbits[mask32 & 7];         // mov     byte [edx], cl
                                                        // mov     ecx, eax
                                                        // shr     ecx, 0x03
                                                        // and     ecx, byte 0x07
                                                        // mov     cl, byte [ecx+esi]
                    tbits[1] = cbits[(mask32>>3) & 7];  // mov     byte [edx+0x1], cl
                                                        // mov     ecx, eax
                                                        // shr     ecx, 0x06
                                                        // and     ecx, byte 0x07
                                                        // mov     cl, byte [ecx+esi]
                    tbits[2] = cbits[(mask32>>6) & 7];  // mov     byte [edx+0x2], cl
                                                        // mov     ecx, eax
                                                        // shr     ecx, 0x09
                                                        // and     ecx, byte 0x07
                    tbits[3] = cbits[(mask32>>9) & 7];  // mov     cl, byte [ecx+esi]
                    mask32 >>= 12;                      // shr     eax, 0x0C
                                                        // mov     byte [edx+0x3], cl
                    tbits += d;                         // add     edx, ebx
                                                        // cmp     edi, byte 0x01
                    if (i == 1) {                       // jnz     short L0026B031
                                                        // xor     ecx, ecx
                        m32a = *((ushort*)ebp);         // mov     cx, word [ebp]
                        mask32 |= m32a << 8;            // shl    ecx, 0x08
                        ebp += 0x02;                    // add     ebp, byte 0x02
                                                        // or      eax, ecx
                    } //    L0026B031:
                                                        // inc     edi
                                                        // cmp     edi, byte 0x04
                }                                   // jge     near L0026B1FF
                break;                              // jmp     short L0026AFE5
            } //    L0026B03D:
                                                        // xor     edx, edx
            tbits = bits;                               // mov     edi, dword [esp+0x1C]
                                                        // mov     dx, word [ebp]
                                                        // mov     ebx, eax
            m32a = *((ushort*)ebp);                     // mov     eax, edx
                                                        // mov     edx, dword [esp+0x30]
            ebp += 0x02;                                // add     ebp, byte 0x02
                                                        // shl     eax, 0x08
            m32a = (m32a << 8) | 0x80000000;            // or      eax, dword 0x80000000
            al0 = 2;                                    // mov     al, byte 0x02
            while (al0 != 0 || m32a != 0) { //    L0026B05C:
                                                        // mov     ecx, ebx
                                                        // and     ecx, byte 0x07
                                                        // test    ecx, ecx
                if (mask32 & 7) {                       // jz      short L0026B068
                                                        // mov     cl, byte [esi+ecx]
                    tbits[0] = cbits[mask32 & 0x007];   // mov     byte [edi], cl
                } //    L0026B068:
                                                        // mov     ecx, ebx
                                                        // and     ecx, byte 0x38
                                                        // test    ecx, ecx
                if (mask32 & 0x38) {                    // jz      short L0026B078
                                                        // shr     ecx, 0x03
                                                        // mov     cl, byte [esi+ecx]
                    tbits[1] = cbits[(mask32 & 0x038) >> 3]; // mov     byte [edi+0x1], cl
                } //    L0026B078:
                                                        // mov     ecx, ebx
                                                        // and     ecx, dword 0x000001C0
                                                        // test    ecx, ecx
                if (mask32 & 0x1c0) {                   // jz      short L0026B08B
                                                        // shr     ecx, 0x06
                                                        // mov     cl, byte [esi+ecx]
                    tbits[2] = cbits[(mask32 & 0x1c0) >> 6]; // mov     byte [edi+0x2], cl
                } //    L0026B08B:
                                                        // mov     ecx, ebx
                                                        // and     ecx, dword 0x00000E00
                                                        // test    ecx, ecx
                if (mask32 & 0xe00) {                   // jz      short L0026B09E
                                                        // shr     ecx, 0x09
                                                        // mov     cl, byte [esi+ecx]
                    tbits[3] = cbits[(mask32 & 0xe00) >> 9]; // mov     byte [edi+0x3], cl
                } //    L0026B09E:
                tbits += d;                             // add     edi, edx
                mask32 >>= 12;                          // shr     ebx, 0x0C
                al0--;                                  // dec     al
                                                        // jnz     short L0026B05C
                                                        // test    eax, eax
                if (al0 == 0 && m32a != 0) {            // jz      short L0026B0B9
                                                        // and     eax, dword 0x00FFFF00
                    mask32 |= m32a & 0x00ffff00;        // or      ebx, eax
                    m32a = 0;
                    al0 = 2;                            // mov     eax, dword 0x00000002
                }                                       // jmp     short L0026B05C
            } //    L0026B0B9:
            break;                                      // jmp     near L0026B1FF
        case 4: //    L0026B0BE:
                                                        // mov     ebx, dword [esp+0x4]
                                                        // mov     edx, dword [_d4x4_colorset]
                                                        // add     ebx, edx
            esp_2c = d4x4_colorset + esp_04;            // mov     dword [esp+0x2C], ebx
            bl0 = *esp_2c;                              // mov     bl, byte [ebx]
            tbits = bits;                               // mov     eax, dword [esp+0x1C]
                                                        // test    bl, bl
            if (bl0) {                                  // jz      short L0026B12D
                                                        // mov     ebx, dword 0x0000000F
                                                        // xor     edi, edi
                for (i = 0; i < 4; ++i) { //    L0026B0DF:
                                                        // xor     edx, edx
                                                        // mov     dl, byte [ebp]
                                                        // mov     ecx, edx
                                                        // mov     esi, dword [esp+0x2C]
                                                        // and     ecx, ebx
                                                        // add     ecx, esi
                                                        // shr     edx, 0x04
                    cl0 = esp_2c[*ebp & 0xf];           // mov     cl, byte [ecx]
                                                        // add     edx, esi
                                                        // mov     byte [eax], cl
                    *(tbits++) = cl0;                   // inc     eax
                    dl0 = esp_2c[*ebp >> 4];            // mov     dl, byte [edx]
    ebp += 1;                                           // inc     ebp
                                                        // mov     byte [eax], dl
                                                        // xor     edx, edx
                                                        // inc     edi
                                                        // mov     dl, byte [ebp]
                    *(tbits++) = dl0;                   // inc     eax
                                                        // mov     ecx, edx
                    mask32 = *ebp++;                    // inc     ebp
                                                        // and     ecx, ebx
                                                        // inc     eax
                                                        // add     ecx, esi
                                                        // shr     edx, 0x04
                    cl0 = esp_2c[mask32 & 0xf];         // mov     cl, byte [ecx]
                                                        // add     edx, esi
                    *(tbits++) = cl0;                   // mov     byte [eax-0x1], cl
                                                        // inc     eax
                    dl0 = esp_2c[mask32 >> 4];          // mov     dl, byte [edx]
                                                        // mov     esi, dword [esp+0x20]
                    *(tbits++) = dl0;                   // mov     byte [eax-0x1], dl
                    tbits += esp_20;                    // add     eax, esi
                                                        // cmp     edi, byte 0x04
                }                                       // jge     near L0026B1FF
                break;                                  // jmp     short L0026B0DF
            } //    L0026B12D:
                                                        // mov     esi, dword [esp+0x2C]
                                                        // mov     edx, dword [esp+0x30]
                                                        // mov     edi, eax
            mask32 = *((uint32_t*)ebp);                 // mov     ebx, dword [ebp]
            for (i = 2; i != 0; --i) {                  // mov     eax, dword 0x00000002
//L0026B13F:
                                                        // mov     ecx, ebx
                                                        // and     ecx, byte 0x0F
                                                        // test    ecx, ecx
                if (mask32 & 0x000f) {                  // jz      short L0026B14B
                                                        // mov     cl, byte [esi+ecx]
                    tbits[0] = esp_2c[mask32 & 0x000f]; // mov     byte [edi], cl
                } //    L0026B14B:
                                                        // mov     ecx, ebx
                                                        // and     ecx, dword 0x000000F0
                                                        // test    ecx, ecx
                if (mask32 & 0x00f0) {                  // jz      short L0026B15E
                                                        // shr     ecx, 0x04
                                                        // mov     cl, byte [esi+ecx]
                    tbits[1] = esp_2c[(mask32 & 0x00f0) >> 4]; // mov     byte [edi+0x1], cl
                } //    L0026B15E:
                                                        // mov     ecx, ebx
                                                        // and     ecx, dword 0x00000F00
                                                        // test    ecx, ecx
                if (mask32 & 0x0f00) {                  // jz      short L0026B171
                                                        // shr     ecx, 0x08
                                                        // mov     cl, byte [esi+ecx]
                    tbits[2] = esp_2c[(mask32 & 0x0f00) >> 8]; // mov     byte [edi+0x2], cl
                } //    L0026B171:
                                                        // mov     ecx, ebx
                                                        // and     ecx, dword 0x0000F000
                                                        // test    ecx, ecx
                if (mask32 & 0xf000) {                  // jz      short L0026B184
                                                        // shr     ecx, 0x0C
                                                        // mov     cl, byte [esi+ecx]
                    tbits[3] = esp_2c[(mask32 & 0xf000) >> 12]; // mov     byte [edi+0x3], cl
                } //    L0026B184:
                tbits += d;                             // add     edi, edx
                mask32 >>= 10;                          // shr     ebx, 0x10
                                                        // dec     eax
            }                                           // jnz     short L0026B13F
            tbits = esp_14;                             // mov     edi, dword [esp+0x14]
                                                        // mov     edx, dword [esp+0x30]
            mask32 = *((uint32_t*)(ebp+0x4));           // mov     ebx, dword [ebp+0x4]
            ebp += 0x08;                                // add     ebp, byte 0x08
            for (i = 2; i != 0; --i) {                  // mov     eax, dword 0x00000002
//L0026B19F:
                                                        // mov     ecx, ebx
                                                        // and     ecx, byte 0x0F
                                                        // test    ecx, ecx
                if (mask32 & 0x000f) {                  // jz      short L0026B1AB
                                                        // mov     cl, byte [esi+ecx]
                    tbits[0] = esp_2c[mask32 & 0x000f]; // mov     byte [edi], cl
                } //    L0026B1AB:
                                                        // mov     ecx, ebx
                                                        // and     ecx, dword 0x000000F0
                                                        // test    ecx, ecx
                if (mask32 & 0x00f0) {                  // jz      short L0026B1BE
                                                        // shr     ecx, 0x04
                                                        // mov     cl, byte [esi+ecx]
                    tbits[1] = esp_2c[(mask32 & 0x00f0) >> 4]; // mov     byte [edi+0x1], cl
                } //    L0026B1BE:
                                                        // mov     ecx, ebx
                                                        // and     ecx, dword 0x00000F00
                                                        // test    ecx, ecx
                if (mask32 & 0x0f00) {                  // jz      short L0026B1D1
                                                        // shr     ecx, 0x08
                                                        // mov     cl, byte [esi+ecx]
                    tbits[2] = esp_2c[(mask32 & 0x0f00) >> 8]; // mov     byte [edi+0x2], cl
                } //    L0026B1D1:
                                                        // mov     ecx, ebx
                                                        // and     ecx, dword 0x0000F000
                                                        // test    ecx, ecx
                if (mask32 & 0xf000) {                  // jz      short L0026B1E4
                                                        // shr     ecx, 0x0C
                                                        // mov     cl, byte [esi+ecx]
                    tbits[3] = esp_2c[(mask32 & 0xf000) >> 12]; // mov     byte [edi+0x3], cl
                } //    L0026B1E4:
                tbits += d;                             // add     edi, edx
                mask32 >>= 0x10;                        // shr     ebx, 0x10
                                                        // dec     eax
            }                                           // jnz     short L0026B19F
            break;                                      // jmp     short L0026B1FF
        case 5: //    L0026B1EE:
                                                        // mov     eax, dword [esp+0x4]
                                                        // mov     edx, dword [esp+0x1C]
                                                        // shl     eax, 0x02
                                                        // add     edx, eax
            bits += esp_04 * 4;                     // mov     dword [esp+0x1C], edx
            break;
        } //    L0026B1FF:
                                                        // mov     ebx, dword [esp+0x18]
                                                        // mov     ecx, dword [esp+0x1C]
                                                        // mov     esi, dword [esp+0x10]
                                                        // add     ebx, byte 0x04
                                                        // add     ecx, byte 0x04
        ++xtab;                                         // mov     dword [esp+0x18], ebx
        bits += 4;                                      // mov     dword [esp+0x1C], ecx
                                                        // cmp     ecx, esi
                                                        // jc      near outer_loop
    } //L0026B221:
                                                        // mov     eax, ebp
    //esp += 0x38;                                      // add     esp, byte 0x38
    //ebp = pop();                                      // pop     ebp
    //edi = pop();                                      // pop     edi
    //esi = pop();                                      // pop     esi
    //popra();
    return ebp;                                         // ret     word 0x0004
}
