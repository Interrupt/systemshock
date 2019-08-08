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
static int d4x4_gamma;
static void* d4x4_beta;

static uchar** d4x4_eptr = &bitstream; // FIXME: should be ptr to something
static int* d4x4_gptr = &d4x4_gamma; // FIXME: should be ptr to something

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

static void Draw4x4_InternalAlpha(int* xtab, int b)
{
    int ebx, esi, edi, ecx, ebp;
    int eax = (int)xtab;
    int edx = b;
    
    char stack[0x08];
    int esp = (int)stack;
    
    //pushra();
    //push(ebx);                                        // push    ebx
    //push(ecx);                                        // push    ecx
    //push(esi);                                        // push    esi
    //push(edi);                                        // push    edi
    //push(ebp);                                        // push    ebp
    //esp -= 0x08;                                      // sub     esp, byte 0x08
    ebx = eax;                                          // mov     ebx, eax
    eax = (int)d4x4_gptr;                               // mov     eax, dword [_d4x4_gptr]
    edi = esp;                                          // mov     edi, esp
    esi = (int)d4x4_eptr;                               // mov     esi, dword [_d4x4_eptr]
    eax = *((int*)(eax));                               // mov     eax, dword [eax]
    movsd();                                            // movsd
    movsd();                                            // movsd
L0026AABF:
                                                        // test    edx, edx
    if (edx <= 0) goto L0026ACE6;                       // jle     near L0026ACE6
                                                        // cmp     edx, byte 0x01
    if (edx <= 0x01) goto L0026AAD5;                    // jle     short L0026AAD5
                                                        // cmp     dword [esp+0x4], byte 0x18
    if (*((int*)(esp+0x4)) > 0x18) goto L0026AB1B;      // jg      short L0026AB1B
    goto L0026AB01;                                     // jmp     short L0026AB01
L0026AAD5:
    edi = *((int*)(esp+0x4));                           // mov     edi, dword [esp+0x4]
                                                        // cmp     edi, byte 0x0C
    if (edi >= 0x0C) goto L0026AB7A;                    // jge     near L0026AB7A
    ecx = *((int*)(esp));                               // mov     ecx, dword [esp]
    ebp = ecx+0x1;                                      // lea     ebp, [ecx+0x1]
    *((int*)(esp)) = ebp;                               // mov     dword [esp], ebp
    ecx = *((char*)(ecx));                              // mov     cl, byte [ecx]
    ecx &= 0x000000FF;                                  // and     ecx, dword 0x000000FF
    eax <<= 0x08;                                       // shl     eax, 0x08
    eax |= ecx;                                         // or      eax, ecx
    ecx = edi+0x8;                                      // lea     ecx, [edi+0x8]
    *((int*)(esp+0x4)) = ecx;                           // mov     dword [esp+0x4], ecx
    goto L0026AAD5;                                     // jmp     short L0026AAD5
L0026AB01:
    edi = esp;                                          // mov     edi, esp
    ecx = *((int*)(edi+0x4));                           // mov     ecx, dword [edi+0x4]
    esi = *((int*)(edi));                               // mov     esi, dword [edi]
L0026AB08:
    eax <<= 0x08;                                       // shl     eax, 0x08
    eax |= *((uchar*)(esi));                            // mov     al, byte [esi]
    esi += 1;                                           // inc     esi
    ecx += 0x08;                                        // add     ecx, byte 0x08
                                                        // cmp     ecx, byte 0x19
    if (ecx < 0x19) goto L0026AB08;                     // jl      short L0026AB08
    *((int*)(edi+0x4)) = ecx;                           // mov     dword [edi+0x4], ecx
    *((int*)(edi)) = esi;                               // mov     dword [edi], esi
L0026AB1B:
    ecx = *((int*)(esp+0x4));                           // mov     ecx, dword [esp+0x4]
    edi = eax;                                          // mov     edi, eax
    ecx -= 0x0C;                                        // sub     ecx, byte 0x0C
    edi = (uint)edi >> ecx;                             // shr     edi, cl
    edi &= 0x00000FFF;                                  // and     edi, dword 0x00000FFF
    ecx = edi*4;                                        // lea     ecx, [edi*4]
    esi = (int)d4x4_hufftab;                            // mov     esi, dword [_d4x4_hufftab]
    ecx -= edi;                                         // sub     ecx, edi
    ecx += esi;                                         // add     ecx, esi
    esi = *((int*)(ecx));                               // mov     esi, dword [ecx]
    edi = esi;                                          // mov     edi, esi
    edi = (uint)edi >> 0x14;                            // shr     edi, 0x14
    edi &= 0x0F;                                        // and     edi, byte 0x0F
                                                        // test    edi, edi
    if (edi == 0) goto L0026AC00;                       // jz      near L0026AC00
    ebp = *((int*)(esp+0x4));                           // mov     ebp, dword [esp+0x4]
    ebp -= edi;                                         // sub     ebp, edi
    edi = esi;                                          // mov     edi, esi
    ebx += 0x04;                                        // add     ebx, byte 0x04
    edi &= 0x00FFFFFF;                                  // and     edi, dword 0x00FFFFFF
    edx -= 1;                                           // dec     edx
    ecx = edi;                                          // mov     ecx, edi
    *((int*)(esp+0x4)) = ebp;                           // mov     dword [esp+0x4], ebp
    ecx &= 0x000E0000;                                  // and     ecx, dword 0x000E0000
    *((int*)(ebx-0x4)) = edi;                           // mov     dword [ebx-0x4], edi
                                                        // cmp     ecx, dword 0x000A0000
    if ((uint)(ecx) >= (uint)(0x000A0000)) goto L0026ABEA;// jnc     near L0026ABEA
L0026AB7A:
    ecx = *((int*)(esp+0x4));                           // mov     ecx, dword [esp+0x4]
    esi = eax;                                          // mov     esi, eax
    ecx -= 0x0C;                                        // sub     ecx, byte 0x0C
    esi = (uint)esi >> ecx;                             // shr     esi, cl
    ecx = esi;                                          // mov     ecx, esi
    ecx &= 0x00000FFF;                                  // and     ecx, dword 0x00000FFF
    edi = ecx*4;                                        // lea     edi, [ecx*4]
    edi -= ecx;                                         // sub     edi, ecx
    ecx = (int)d4x4_hufftab;                            // mov     ecx, dword [_d4x4_hufftab]
    ecx += edi;                                         // add     ecx, edi
    edi = *((int*)(ecx));                               // mov     edi, dword [ecx]
    esi = edi;                                          // mov     esi, edi
    esi = (uint)esi >> 0x14;                            // shr     esi, 0x14
    esi &= 0x0F;                                        // and     esi, byte 0x0F
                                                        // test    esi, esi
    if (esi == 0) goto L0026AC00;                       // jz      short L0026AC00
    ecx = *((int*)(esp+0x4));                           // mov     ecx, dword [esp+0x4]
    edi &= 0x00FFFFFF;                                  // and     edi, dword 0x00FFFFFF
    ecx -= esi;                                         // sub     ecx, esi
    ebx += 0x04;                                        // add     ebx, byte 0x04
    *((int*)(esp+0x4)) = ecx;                           // mov     dword [esp+0x4], ecx
    ecx = edi;                                          // mov     ecx, edi
    edx -= 1;                                           // dec     edx
    ecx &= 0x000E0000;                                  // and     ecx, dword 0x000E0000
    *((int*)(ebx-0x4)) = edi;                           // mov     dword [ebx-0x4], edi
                                                        // cmp     ecx, dword 0x000A0000
    if ((uint)(ecx) >= (uint)(0x000A0000)) goto L0026ABEA;// jnc     short L0026ABEA
                                                        // test    edx, edx
    if (edx == 0) goto L0026ABEA;                       // jz      short L0026ABEA
                                                        // cmp     dword [esp+0x4], byte 0x0C
    if (*((int*)(esp+0x4)) >= 0x0C) goto L0026AB7A;     // jge     short L0026AB7A
                                                        // cmp     edx, byte 0x01
    if (edx > 0x01) goto L0026AB01;                     // jg      near L0026AB01
    goto L0026AAD5;                                     // jmp     near L0026AAD5
L0026ABEA:
    ecx = edi;                                          // mov     ecx, edi
    ecx &= 0x000E0000;                                  // and     ecx, dword 0x000E0000
                                                        // cmp     ecx, dword 0x000A0000
    if ((uint)(ecx) < (uint)(0x000A0000)) goto L0026ACE6;// jc      near L0026ACE6
    goto L0026AC6A;                                     // jmp     short L0026AC6A
L0026AC00:
    esi = (int)d4x4_gptr;                               // mov     esi, dword [_d4x4_gptr]
    edi = (int)d4x4_eptr;                               // mov     edi, dword [_d4x4_eptr]
    *((int*)(esi)) = eax;                               // mov     dword [esi], eax
    esi = esp;                                          // mov     esi, esp
    eax = ecx;                                          // mov     eax, ecx
    movsd();                                            // movsd
    movsd();                                            // movsd
    eax = (int)Draw4x4_InternalGamma((uchar*)eax);      // call    near Draw4x4_InternalGamma_
    ecx = eax;                                          // mov     ecx, eax
    eax = (int)d4x4_gptr;                               // mov     eax, dword [_d4x4_gptr]
    edi = esp;                                          // mov     edi, esp
    esi = (int)d4x4_eptr;                               // mov     esi, dword [_d4x4_eptr]
    eax = *((int*)(eax));                               // mov     eax, dword [eax]
    movsd();                                            // movsd
    movsd();                                            // movsd
    esi = *((int*)(ecx));                               // mov     esi, dword [ecx]
    esi = (uint)esi >> 0x14;                            // shr     esi, 0x14
    edi = *((int*)(esp+0x4));                           // mov     edi, dword [esp+0x4]
    esi &= 0x0F;                                        // and     esi, byte 0x0F
    edi -= esi;                                         // sub     edi, esi
    *((int*)(esp+0x4)) = edi;                           // mov     dword [esp+0x4], edi
    edi = *((int*)(ecx));                               // mov     edi, dword [ecx]
    edi &= 0x00FFFFFF;                                  // and     edi, dword 0x00FFFFFF
    ebx += 0x04;                                        // add     ebx, byte 0x04
    ecx = edi;                                          // mov     ecx, edi
    edx -= 1;                                           // dec     edx
    ecx &= 0x000E0000;                                  // and     ecx, dword 0x000E0000
    *((int*)(ebx-0x4)) = edi;                           // mov     dword [ebx-0x4], edi
                                                        // cmp     ecx, dword 0x000A0000
    if ((uint)(ecx) >= (uint)(0x000A0000)) goto L0026AC6A;// jnc     short L0026AC6A
                                                        // test    edx, edx
    if (edx == 0) goto L0026ACE6;                       // jz      near L0026ACE6
    goto L0026AABF;                                     // jmp     near L0026AABF
L0026AC6A:
    edi &= 0x000E0000;                                  // and     edi, dword 0x000E0000
    ebx -= 0x04;                                        // sub     ebx, byte 0x04
                                                        // cmp     edi, dword 0x000A0000
    if (edi != 0x000A0000) goto L0026ACD8;              // jnz     short L0026ACD8
    ebp = *((int*)(esp+0x4));                           // mov     ebp, dword [esp+0x4]
                                                        // cmp     ebp, byte 0x05
    if (ebp >= 0x05) goto L0026ACA1;                    // jge     short L0026ACA1
    ecx = *((int*)(esp));                               // mov     ecx, dword [esp]
    esi = ecx+0x1;                                      // lea     esi, [ecx+0x1]
    *((int*)(esp)) = esi;                               // mov     dword [esp], esi
    ecx = *((char*)(ecx));                              // mov     cl, byte [ecx]
    ecx &= 0x000000FF;                                  // and     ecx, dword 0x000000FF
    edi = ebp+0x8;                                      // lea     edi, [ebp+0x8]
    eax <<= 0x08;                                       // shl     eax, 0x08
    *((int*)(esp+0x4)) = edi;                           // mov     dword [esp+0x4], edi
    eax |= ecx;                                         // or      eax, ecx
L0026ACA1:
    ecx = *((int*)(esp+0x4));                           // mov     ecx, dword [esp+0x4]
    esi = eax;                                          // mov     esi, eax
    ecx -= 0x05;                                        // sub     ecx, byte 0x05
    esi = (uint)esi >>  ecx;                            // shr     esi, cl
    ebp = *((int*)(esp+0x4));                           // mov     ebp, dword [esp+0x4]
    ecx = esi;                                          // mov     ecx, esi
    ebp -= 0x05;                                        // sub     ebp, byte 0x05
    ecx &= 0x1F;                                        // and     ecx, byte 0x1F
    *((int*)(esp+0x4)) = ebp;                           // mov     dword [esp+0x4], ebp
                                                        // cmp     ecx, byte 0x1F
    if (ecx != 0x1F) goto L0026ACC3;                    // jnz     short L0026ACC3
    ecx = edx;                                          // mov     ecx, edx
L0026ACC3:
    esi = ecx;                                          // mov     esi, ecx
    ebx += 0x04;                                        // add     ebx, byte 0x04
    esi |= 0x000A0000;                                  // or      esi, dword 0x000A0000
    edx -= ecx;                                         // sub     edx, ecx
    *((int*)(ebx-0x4)) = esi;                           // mov     dword [ebx-0x4], esi
    goto L0026AABF;                                     // jmp     near L0026AABF
L0026ACD8:
    ebx += 0x04;                                        // add     ebx, byte 0x04
    ecx = *((int*)(ebx-0x8));                           // mov     ecx, dword [ebx-0x8]
    *((int*)(ebx-0x4)) = ecx;                           // mov     dword [ebx-0x4], ecx
    goto L0026AABF;                                     // jmp     near L0026AABF
L0026ACE6:
    edx = (int)d4x4_gptr;                               // mov     edx, dword [_d4x4_gptr]
    esi = esp;                                          // mov     esi, esp
    edi = (int)d4x4_eptr;                               // mov     edi, dword [_d4x4_eptr]
    *((int*)(edx)) = eax;                               // mov     dword [edx], eax
    movsd();                                            // movsd
    movsd();                                            // movsd
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

static uchar* Draw4x4_InternalBeta(int* xtab, int b, uchar* bits, int d, uchar* mask_stream)
{
    char cl0, dl0, bl0;
    char al0, ah0;
    
    int esi, edi, ebp;
    int eax = (int)xtab;
    int edx = b;
    int ebx = (int)bits;
    int ecx = d;
    char stack[0x38];
    int esp = (int)stack;
    
    //pushra();
    //push(esi);                                        // push    esi
    //push(edi);                                        // push    edi
    //push(ebp);                                        // push    ebp
    //esp -= 0x38;                                      // sub     esp, byte 0x38
    //ebp = *((int*)(esp+0x48));                        // mov     ebp, dword [esp+0x48]
    ebp = (int)mask_stream;
    *((int*)(esp+0x18)) = eax;                          // mov     dword [esp+0x18], eax
    *((int*)(esp+0x1C)) = ebx;                          // mov     dword [esp+0x1C], ebx
    *((int*)(esp+0x30)) = ecx;                          // mov     dword [esp+0x30], ecx
    eax = ecx-0x4;                                      // lea     eax, [ecx-0x4]
    edx <<= 0x02;                                       // shl     edx, 0x02
    *((int*)(esp+0x20)) = eax;                          // mov     dword [esp+0x20], eax
    eax = ebx+edx;                                      // lea     eax, [ebx+edx]
    *((int*)(esp+0x10)) = eax;                          // mov     dword [esp+0x10], eax
                                                        // cmp     ebx, eax
    if ((uint)(ebx) >= (uint)(eax)) goto L0026B221;     // jnc     near L0026B221
    eax = ecx*4;                                        // lea     eax, [ecx*4]
    eax -= ecx;                                         // sub     eax, ecx
    *((int*)(esp+0xC)) = eax;                           // mov     dword [esp+0xC], eax
    eax = ecx+ecx;                                      // lea     eax, [ecx+ecx]
    *((int*)(esp+0x8)) = eax;                           // mov     dword [esp+0x8], eax
outer_loop:
    eax = *((int*)(esp+0x18));                          // mov     eax, dword [esp+0x18]
    eax = *((int*)(eax));                               // mov     eax, dword [eax]
    eax &= 0x0001FFFF;                                  // and     eax, dword 0x0001FFFF
    *((int*)(esp+0x4)) = eax;                           // mov     dword [esp+0x4], eax
    eax = *((int*)(esp+0x18));                          // mov     eax, dword [esp+0x18]
    eax = *((int*)(eax));                               // mov     eax, dword [eax]
    asm_shr(eax, 0x11);                                 // shr     eax, 0x11
    eax &= 0x07;                                        // and     eax, byte 0x07
                                                        // cmp     eax, byte 0x05
    if ((uint)(eax) > (uchar)(0x05)) goto L0026B1FF;    // ja      near L0026B1FF
    edx = *((int*)(esp+0x1C));                          // mov     edx, dword [esp+0x1C]
    edi = *((int*)(esp+0x8));                           // mov     edi, dword [esp+0x8]
    edx += edi;                                         // add     edx, edi
    *((int*)(esp+0x14)) = edx;                          // mov     dword [esp+0x14], edx
    edx = ebp+0x4;                                      // lea     edx, [ebp+0x4]
    *((int*)(esp)) = edx;                               // mov     dword [esp], edx
    
    if (eax == 0)     goto L0026AD9F;
    if (eax == 1)     goto L0026ADD1;
    if (eax == 2)     goto L0026AE92;
    if (eax == 3)     goto L0026AFC3;
    if (eax == 4)     goto L0026B0BE;
    /*if (eax == 5)*/ goto L0026B1EE;
                                                        // jmp     dword [cs:eax*4+L0026AD04]
L0026AD9F:
    eax = *((int*)(esp+0x4));                           // mov     eax, dword [esp+0x4]
    esi = *((int*)(esp+0x4));                           // mov     esi, dword [esp+0x4]
    eax <<= 0x10;                                       // shl     eax, 0x10
    edx = *((int*)(esp+0x1C));                          // mov     edx, dword [esp+0x1C]
    eax |= esi;                                         // or      eax, esi
    edi = *((int*)(esp+0x30));                          // mov     edi, dword [esp+0x30]
    *((int*)(edx)) = eax;                               // mov     dword [edx], eax
    edx += edi;                                         // add     edx, edi
    *((int*)(edx)) = eax;                               // mov     dword [edx], eax
    edx = *((int*)(esp+0x14));                          // mov     edx, dword [esp+0x14]
    ebx = *((int*)(esp+0xC));                           // mov     ebx, dword [esp+0xC]
    *((int*)(edx)) = eax;                               // mov     dword [edx], eax
    edx = *((int*)(esp+0x1C));                          // mov     edx, dword [esp+0x1C]
    edx += ebx;                                         // add     edx, ebx
    *((int*)(edx)) = eax;                               // mov     dword [edx], eax
    goto L0026B1FF;                                     // jmp     near L0026B1FF
L0026ADD1:
    eax = *((int*)(esp+0x1C));                          // mov     eax, dword [esp+0x1C]
    esi = esp+0x4;                                      // lea     esi, [esp+0x4]
    ebp += 0x02;                                        // add     ebp, byte 0x02
    edx = 0;                                            // xor     edx, edx
    //ch = *((char*)(esp+0x4));                         // mov     ch, byte [esp+0x4]
    edx = *((ushort*)(ebp-0x2));                        // mov     dx, word [ebp-0x2]
                                                        // test    ch, ch
    if (*((char*)(esp+0x4)) == 0) goto L0026AE34;       // jz      short L0026AE34
    ebx = *((int*)(esp+0x30));                          // mov     ebx, dword [esp+0x30]
    edi = 0;                                            // xor     edi, edi
L0026ADF0:
    ecx = edx;                                          // mov     ecx, edx
    ecx &= 0x01;                                        // and     ecx, byte 0x01
    cl0 = *((char*)(ecx+esi));                          // mov     cl, byte [ecx+esi]
    *((char*)(eax)) = cl0;                              // mov     byte [eax], cl
    ecx = edx;                                          // mov     ecx, edx
    asm_shr(ecx, 0x00000001);                           // shr     ecx, 0x00000001
    ecx &= 0x01;                                        // and     ecx, byte 0x01
    cl0 = *((char*)(ecx+esi));                          // mov     cl, byte [ecx+esi]
    *((char*)(eax+0x1)) = cl0;                          // mov     byte [eax+0x1], cl
    ecx = edx;                                          // mov     ecx, edx
    asm_shr(ecx, 0x02);                                 // shr     ecx, 0x02
    ecx &= 0x01;                                        // and     ecx, byte 0x01
    cl0 = *((char*)(ecx+esi));                          // mov     cl, byte [ecx+esi]
    *((char*)(eax+0x2)) = cl0;                          // mov     byte [eax+0x2], cl
    ecx = edx;                                          // mov     ecx, edx
    asm_shr(ecx, 0x03);                                 // shr     ecx, 0x03
    ecx &= 0x01;                                        // and     ecx, byte 0x01
    edi += 1;                                           // inc     edi
    cl0 = *((char*)(ecx+esi));                          // mov     cl, byte [ecx+esi]
    asm_shr(edx, 0x04);                                 // shr     edx, 0x04
    *((char*)(eax+0x3)) = cl0;                          // mov     byte [eax+0x3], cl
    eax += ebx;                                         // add     eax, ebx
                                                        // cmp     edi, byte 0x04
    if (edi >= 0x04) goto L0026B1FF;                    // jge     near L0026B1FF
    goto L0026ADF0;                                     // jmp     short L0026ADF0
L0026AE34:
    ecx = 0;                                            // xor     ecx, ecx
    ecx = *((uchar*)(esp+0x5));                         // mov     cl, byte [esp+0x5]
    ebx = ecx;                                          // mov     ebx, ecx
    edi = 0x00000004;                                   // mov     edi, dword 0x00000004
    ebx <<= 0x08;                                       // shl     ebx, 0x08
    esi = *((int*)(esp+0x30));                          // mov     esi, dword [esp+0x30]
    ecx |= ebx;                                         // or      ecx, ebx
L0026AE4A:
                                                        // test    dl, byte 0x03
    if ((edx & 0x03) == 0) goto L0026AE65;              // jz      short L0026AE65
                                                        // test    dl, byte 0x01
    if ((edx & 0x01) == 0) goto L0026AE62;              // jz      short L0026AE62
                                                        // test    dl, byte 0x02
    if ((edx & 0x02) == 0) goto L0026AE5E;              // jz      short L0026AE5E
    *((ushort*)(eax)) = (ushort)ecx;                    // mov     word [eax], cx
    goto L0026AE65;                                     // jmp     short L0026AE65
L0026AE5E:
    *((uchar*)(eax)) = (uchar)ecx;                      // mov     byte [eax], cl
    goto L0026AE65;                                     // jmp     short L0026AE65
L0026AE62:
    *((uchar*)(eax+0x1)) = (uchar)ecx;                  // mov     byte [eax+0x1], cl
L0026AE65:
                                                        // test    dl, byte 0x0C
    if ((edx & 0x0C) == 0) goto L0026AE82;              // jz      short L0026AE82
                                                        // test    dl, byte 0x04
    if ((edx & 0x04) == 0) goto L0026AE7F;              // jz      short L0026AE7F
                                                        // test    dl, byte 0x08
    if ((edx & 0x08) == 0) goto L0026AE7A;              // jz      short L0026AE7A
    *((ushort*)(eax+0x2)) = (ushort)ecx;                // mov     word [eax+0x2], cx
    goto L0026AE82;                                     // jmp     short L0026AE82
L0026AE7A:
    *((uchar*)(eax+0x2)) = (uchar)ecx;                  // mov     byte [eax+0x2], cl
    goto L0026AE82;                                     // jmp     short L0026AE82
L0026AE7F:
    *((uchar*)(eax+0x3)) = (uchar)ecx;                  // mov     byte [eax+0x3], cl
L0026AE82:
    asm_shr(edx, 0x04);                                 // shr     edx, 0x04
    edi -= 1;                                           // dec     edi
    eax += esi;                                         // add     eax, esi
                                                        // test    edi, edi
    if (edi <= 0) goto L0026B1FF;                       // jle     near L0026B1FF
    goto L0026AE4A;                                     // jmp     short L0026AE4A
L0026AE92:
    eax = *((int*)(ebp));                               // mov     eax, dword [ebp]
    *((int*)(esp+0x34)) = eax;                          // mov     dword [esp+0x34], eax
    ebp = edx;                                          // mov     ebp, edx
    edx = *((int*)(esp+0x4));                           // mov     edx, dword [esp+0x4]
    eax = (int)d4x4_colorset;                           // mov     eax, dword [_d4x4_colorset]
    edx += eax;                                         // add     edx, eax
    edi = *((int*)(esp+0x1C));                          // mov     edi, dword [esp+0x1C]
    //cl = *((char*)(edx));                             // mov     cl, byte [edx]
    *((int*)(esp+0x24)) = edx;                          // mov     dword [esp+0x24], edx
                                                        // test    cl, cl
    if (*((char*)(edx)) == 0) goto L0026AF6A;           // jz      near L0026AF6A
    *((int*)(esp+0x28)) = 0x00000004;                   // mov     dword [esp+0x28], dword 0x00000004
L0026AEC0:
    esi = *((int*)(esp+0x24));                          // mov     esi, dword [esp+0x24]
    edx = *((int*)(esp+0x34));                          // mov     edx, dword [esp+0x34]
    ecx = edi;                                          // mov     ecx, edi
    eax = edx;                                          // mov     eax, edx
    eax &= 0x03;                                        // and     eax, byte 0x03
    ebx &= 0xFFFF0000;
    ebx |= *((uchar*)(esi+eax));                        // mov     bl, byte [esi+eax]
    eax = edx;                                          // mov     eax, edx
    asm_shr(eax, 0x02);                                 // shr     eax, 0x02
    eax &= 0x03;                                        // and     eax, byte 0x03
    ebx |= *((uchar*)(esi+eax)) << 8;                   // mov     bh, byte [esi+eax]
    *((ushort*)(ecx)) = ebx & 0xFFFF;                   // mov     word [ecx], bx
    eax = edx;                                          // mov     eax, edx
    asm_shr(eax, 0x04);                                 // shr     eax, 0x04
    eax &= 0x03;                                        // and     eax, byte 0x03
    ebx &= 0xFFFF0000;
    ebx |= *((uchar*)(esi+eax));                        // mov     bl, byte [esi+eax]
    eax = edx;                                          // mov     eax, edx
    asm_shr(eax, 0x06);                                 // shr     eax, 0x06
    eax &= 0x03;                                        // and     eax, byte 0x03
    ebx |= *((uchar*)(esi+eax)) << 8;                   // mov     bh, byte [esi+eax]
    *((ushort*)(ecx+0x2)) = ebx & 0xFFFF;               // mov     word [ecx+0x2], bx
    ecx = edx;                                          // mov     ecx, edx
    esi = *((int*)(esp+0x30));                          // mov     esi, dword [esp+0x30]
    asm_shr(ecx, 0x08);                                 // shr     ecx, 0x08
    edi += esi;                                         // add     edi, esi
    *((int*)(esp+0x34)) = ecx;                          // mov     dword [esp+0x34], ecx
    esi = *((int*)(esp+0x24));                          // mov     esi, dword [esp+0x24]
    edx = *((int*)(esp+0x34));                          // mov     edx, dword [esp+0x34]
    ecx = edi;                                          // mov     ecx, edi
    eax = edx;                                          // mov     eax, edx
    eax &= 0x03;                                        // and     eax, byte 0x03
    ebx &= 0xFFFF0000;
    ebx |= *((uchar*)(esi+eax));                        // mov     bl, byte [esi+eax]
    eax = edx;                                          // mov     eax, edx
    asm_shr(eax, 0x02);                                 // shr     eax, 0x02
    eax &= 0x03;                                        // and     eax, byte 0x03
    ebx |= *((uchar*)(esi+eax)) << 8;                   // mov     bh, byte [esi+eax]
    *((ushort*)(ecx)) = ebx & 0xFFFF;                   // mov     word [ecx], bx
    eax = edx;                                          // mov     eax, edx
    asm_shr(eax, 0x04);                                 // shr     eax, 0x04
    eax &= 0x03;                                        // and     eax, byte 0x03
    ebx &= 0xFFFF0000;
    ebx |= *((uchar*)(esi+eax));                        // mov     bl, byte [esi+eax]
    eax = edx;                                          // mov     eax, edx
    asm_shr(eax, 0x06);                                 // shr     eax, 0x06
    eax &= 0x03;                                        // and     eax, byte 0x03
    ebx |= *((uchar*)(esi+eax)) << 8;                   // mov     bh, byte [esi+eax]
    *((ushort*)(ecx+0x2)) = ebx & 0xFFFF;               // mov     word [ecx+0x2], bx
    ebx = *((int*)(esp+0x28));                          // mov     ebx, dword [esp+0x28]
    eax = edx;                                          // mov     eax, edx
    edx = *((int*)(esp+0x30));                          // mov     edx, dword [esp+0x30]
    asm_shr(eax, 0x08);                                 // shr     eax, 0x08
    ebx -= 0x02;                                        // sub     ebx, byte 0x02
    *((int*)(esp+0x34)) = eax;                          // mov     dword [esp+0x34], eax
    *((int*)(esp+0x28)) = ebx;                          // mov     dword [esp+0x28], ebx
    edi += edx;                                         // add     edi, edx
                                                        // test    ebx, ebx
    if (ebx <= 0) goto L0026B1FF;                       // jle     near L0026B1FF
    goto L0026AEC0;                                     // jmp     near L0026AEC0
L0026AF6A:
    ebx = *((int*)(esp+0x34));                          // mov     ebx, dword [esp+0x34]
    esi = edx;                                          // mov     esi, edx
    edx = *((int*)(esp+0x30));                          // mov     edx, dword [esp+0x30]
    ah0 = 0x04;                                         // mov     ah, byte 0x04
simple_loop:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x03;                                        // and     ecx, byte 0x03
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026AF82;                       // jz      short L0026AF82
    al0 = *((char*)(esi+ecx));                          // mov     al, byte [esi+ecx]
    *((char*)(edi)) = al0;                              // mov     byte [edi], al
L0026AF82:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x0C;                                        // and     ecx, byte 0x0C
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026AF92;                       // jz      short L0026AF92
    asm_shr(ecx, 0x02);                                 // shr     ecx, 0x02
    al0 = *((char*)(esi+ecx));                          // mov     al, byte [esi+ecx]
    *((char*)(edi+0x1)) = al0;                          // mov     byte [edi+0x1], al
L0026AF92:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x30;                                        // and     ecx, byte 0x30
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026AFA2;                       // jz      short L0026AFA2
    asm_shr(ecx, 0x04);                                 // shr     ecx, 0x04
    al0 = *((char*)(esi+ecx));                          // mov     al, byte [esi+ecx]
    *((char*)(edi+0x2)) = al0;                          // mov     byte [edi+0x2], al
L0026AFA2:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x000000C0;                                  // and     ecx, dword 0x000000C0
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026AFB5;                       // jz      short L0026AFB5
    asm_shr(ecx, 0x06);                                 // shr     ecx, 0x06
    al0 = *((char*)(esi+ecx));                          // mov     al, byte [esi+ecx]
    *((char*)(edi+0x3)) = al0;                          // mov     byte [edi+0x3], al
L0026AFB5:
    edi += edx;                                         // add     edi, edx
    asm_shr(ebx, 0x08);                                 // shr     ebx, 0x08
    ah0 -= 1;                                           // dec     ah
    if (ah0 != 0) goto simple_loop;                     // jnz     short simple_loop
    goto L0026B1FF;                                     // jmp     near L0026B1FF
L0026AFC3:
    ebx = *((int*)(esp+0x4));                           // mov     ebx, dword [esp+0x4]
    esi = (int)d4x4_colorset;                           // mov     esi, dword [_d4x4_colorset]
    esi += ebx;                                         // add     esi, ebx
    edx = *((int*)(esp+0x1C));                          // mov     edx, dword [esp+0x1C]
    eax = *((int*)(ebp));                               // mov     eax, dword [ebp]
    //bh = *((char*)(esi));                             // mov     bh, byte [esi]
    ebp = *((int*)(esp));                               // mov     ebp, dword [esp]
                                                        // test    bh, bh
    if (*((char*)(esi)) == 0) goto L0026B03D;           // jz      short L0026B03D
    ebx = *((int*)(esp+0x30));                          // mov     ebx, dword [esp+0x30]
    edi = 0;                                            // xor     edi, edi
L0026AFE5:
    ecx = eax;                                          // mov     ecx, eax
    ecx &= 0x07;                                        // and     ecx, byte 0x07
    cl0 = *((char*)(ecx+esi));                          // mov     cl, byte [ecx+esi]
    *((char*)(edx)) = cl0;                              // mov     byte [edx], cl
    ecx = eax;                                          // mov     ecx, eax
    asm_shr(ecx, 0x03);                                 // shr     ecx, 0x03
    ecx &= 0x07;                                        // and     ecx, byte 0x07
    cl0 = *((char*)(ecx+esi));                          // mov     cl, byte [ecx+esi]
    *((char*)(edx+0x1)) = cl0;                          // mov     byte [edx+0x1], cl
    ecx = eax;                                          // mov     ecx, eax
    asm_shr(ecx, 0x06);                                 // shr     ecx, 0x06
    ecx &= 0x07;                                        // and     ecx, byte 0x07
    cl0 = *((char*)(ecx+esi));                          // mov     cl, byte [ecx+esi]
    *((char*)(edx+0x2)) = cl0;                          // mov     byte [edx+0x2], cl
    ecx = eax;                                          // mov     ecx, eax
    asm_shr(ecx, 0x09);                                 // shr     ecx, 0x09
    ecx &= 0x07;                                        // and     ecx, byte 0x07
    cl0 = *((char*)(ecx+esi));                          // mov     cl, byte [ecx+esi]
    asm_shr(eax, 0x0C);                                 // shr     eax, 0x0C
    *((char*)(edx+0x3)) = cl0;                          // mov     byte [edx+0x3], cl
    edx += ebx;                                         // add     edx, ebx
                                                        // cmp     edi, byte 0x01
    if (edi != 0x01) goto L0026B031;                    // jnz     short L0026B031
    ecx = 0;                                            // xor     ecx, ecx
    ecx = *((ushort*)(ebp));                            // mov     cx, word [ebp]
    ecx <<= 0x08;                                       // shl     ecx, 0x08
    ebp += 0x02;                                        // add     ebp, byte 0x02
    eax |= ecx;                                         // or      eax, ecx
L0026B031:
    edi += 1;                                           // inc     edi
                                                        // cmp     edi, byte 0x04
    if (edi >= 0x04) goto L0026B1FF;                    // jge     near L0026B1FF
    goto L0026AFE5;                                     // jmp     short L0026AFE5
L0026B03D:
    edx = 0;                                            // xor     edx, edx
    edi = *((int*)(esp+0x1C));                          // mov     edi, dword [esp+0x1C]
    edx = *((ushort*)(ebp));                            // mov     dx, word [ebp]
    ebx = eax;                                          // mov     ebx, eax
    eax = edx;                                          // mov     eax, edx
    edx = *((int*)(esp+0x30));                          // mov     edx, dword [esp+0x30]
    ebp += 0x02;                                        // add     ebp, byte 0x02
    eax <<= 0x08;                                       // shl     eax, 0x08
    eax |= 0x80000000;                                  // or      eax, dword 0x80000000
    eax &= 0xFFFFFF00;
    eax |= 0x02;                                        // mov     al, byte 0x02
L0026B05C:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x07;                                        // and     ecx, byte 0x07
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B068;                       // jz      short L0026B068
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi)) = cl0;                              // mov     byte [edi], cl
L0026B068:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x38;                                        // and     ecx, byte 0x38
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B078;                       // jz      short L0026B078
    asm_shr(ecx, 0x03);                                 // shr     ecx, 0x03
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi+0x1)) = cl0;                          // mov     byte [edi+0x1], cl
L0026B078:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x000001C0;                                  // and     ecx, dword 0x000001C0
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B08B;                       // jz      short L0026B08B
    asm_shr(ecx, 0x06);                                 // shr     ecx, 0x06
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi+0x2)) = cl0;                          // mov     byte [edi+0x2], cl
L0026B08B:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x00000E00;                                  // and     ecx, dword 0x00000E00
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B09E;                       // jz      short L0026B09E
    asm_shr(ecx, 0x09);                                 // shr     ecx, 0x09
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi+0x3)) = cl0;                          // mov     byte [edi+0x3], cl
L0026B09E:
    edi += edx;                                         // add     edi, edx
    asm_shr(ebx, 0x0C);                                 // shr     ebx, 0x0C
    eax = (eax & 0xFFFFFF00) + ((eax - 1) & 0xFF);      // dec     al
    if ((eax & 0xFF) != 0) goto L0026B05C;              // jnz     short L0026B05C
                                                        // test    eax, eax
    if (eax == 0) goto L0026B0B9;                       // jz      short L0026B0B9
    eax &= 0x00FFFF00;                                  // and     eax, dword 0x00FFFF00
    ebx |= eax;                                         // or      ebx, eax
    eax = 0x00000002;                                   // mov     eax, dword 0x00000002
    goto L0026B05C;                                     // jmp     short L0026B05C
L0026B0B9:
    goto L0026B1FF;                                     // jmp     near L0026B1FF
L0026B0BE:
    ebx = *((int*)(esp+0x4));                           // mov     ebx, dword [esp+0x4]
    edx = (int)d4x4_colorset;                           // mov     edx, dword [_d4x4_colorset]
    ebx += edx;                                         // add     ebx, edx
    *((int*)(esp+0x2C)) = ebx;                          // mov     dword [esp+0x2C], ebx
    bl0 = *((char*)(ebx));                              // mov     bl, byte [ebx]
    eax = *((int*)(esp+0x1C));                          // mov     eax, dword [esp+0x1C]
                                                        // test    bl, bl
    if (bl0 == 0) goto L0026B12D;                       // jz      short L0026B12D
    ebx = 0x0000000F;                                   // mov     ebx, dword 0x0000000F
    edi = 0;                                            // xor     edi, edi
L0026B0DF:
    edx = 0;                                            // xor     edx, edx
    edx = *((uchar*)(ebp));                             // mov     dl, byte [ebp]
    ecx = edx;                                          // mov     ecx, edx
    esi = *((int*)(esp+0x2C));                          // mov     esi, dword [esp+0x2C]
    ecx &= ebx;                                         // and     ecx, ebx
    ecx += esi;                                         // add     ecx, esi
    asm_shr(edx, 0x04);                                 // shr     edx, 0x04
    cl0 = *((char*)(ecx));                              // mov     cl, byte [ecx]
    edx += esi;                                         // add     edx, esi
    *((char*)(eax)) = cl0;                              // mov     byte [eax], cl
    eax += 1;                                           // inc     eax
    dl0 = *((char*)(edx));                              // mov     dl, byte [edx]
    ebp += 1;                                           // inc     ebp
    *((char*)(eax)) = dl0;                              // mov     byte [eax], dl
    edx = 0;                                            // xor     edx, edx
    edi += 1;                                           // inc     edi
    edx = *((uchar*)(ebp));                             // mov     dl, byte [ebp]
    eax += 1;                                           // inc     eax
    ecx = edx;                                          // mov     ecx, edx
    ebp += 1;                                           // inc     ebp
    ecx &= ebx;                                         // and     ecx, ebx
    eax += 1;                                           // inc     eax
    ecx += esi;                                         // add     ecx, esi
    asm_shr(edx, 0x04);                                 // shr     edx, 0x04
    cl0 = *((char*)(ecx));                              // mov     cl, byte [ecx]
    edx += esi;                                         // add     edx, esi
    *((char*)(eax-0x1)) = cl0;                          // mov     byte [eax-0x1], cl
    eax += 1;                                           // inc     eax
    dl0 = *((char*)(edx));                              // mov     dl, byte [edx]
    esi = *((int*)(esp+0x20));                          // mov     esi, dword [esp+0x20]
    *((char*)(eax-0x1)) = dl0;                          // mov     byte [eax-0x1], dl
    eax += esi;                                         // add     eax, esi
                                                        // cmp     edi, byte 0x04
    if (edi >= 0x04) goto L0026B1FF;                    // jge     near L0026B1FF
    goto L0026B0DF;                                     // jmp     short L0026B0DF
L0026B12D:
    esi = *((int*)(esp+0x2C));                          // mov     esi, dword [esp+0x2C]
    edx = *((int*)(esp+0x30));                          // mov     edx, dword [esp+0x30]
    edi = eax;                                          // mov     edi, eax
    ebx = *((int*)(ebp));                               // mov     ebx, dword [ebp]
    eax = 0x00000002;                                   // mov     eax, dword 0x00000002
L0026B13F:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x0F;                                        // and     ecx, byte 0x0F
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B14B;                       // jz      short L0026B14B
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi)) = cl0;                              // mov     byte [edi], cl
L0026B14B:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x000000F0;                                  // and     ecx, dword 0x000000F0
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B15E;                       // jz      short L0026B15E
    asm_shr(ecx, 0x04);                                 // shr     ecx, 0x04
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi+0x1)) = cl0;                          // mov     byte [edi+0x1], cl
L0026B15E:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x00000F00;                                  // and     ecx, dword 0x00000F00
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B171;                       // jz      short L0026B171
    asm_shr(ecx, 0x08);                                 // shr     ecx, 0x08
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi+0x2)) = cl0;                          // mov     byte [edi+0x2], cl
L0026B171:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x0000F000;                                  // and     ecx, dword 0x0000F000
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B184;                       // jz      short L0026B184
    asm_shr(ecx, 0x0C);                                 // shr     ecx, 0x0C
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi+0x3)) = cl0;                          // mov     byte [edi+0x3], cl
L0026B184:
    edi += edx;                                         // add     edi, edx
    asm_shr(ebx, 0x10);                                 // shr     ebx, 0x10
    eax -= 1;                                           // dec     eax
    if (eax != 0) goto L0026B13F;                       // jnz     short L0026B13F
    edi = *((int*)(esp+0x14));                          // mov     edi, dword [esp+0x14]
    edx = *((int*)(esp+0x30));                          // mov     edx, dword [esp+0x30]
    ebx = *((int*)(ebp+0x4));                           // mov     ebx, dword [ebp+0x4]
    ebp += 0x08;                                        // add     ebp, byte 0x08
    eax = 0x00000002;                                   // mov     eax, dword 0x00000002
L0026B19F:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x0F;                                        // and     ecx, byte 0x0F
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B1AB;                       // jz      short L0026B1AB
    cl0= *((char*)(esi+ecx));                           // mov     cl, byte [esi+ecx]
    *((char*)(edi)) = cl0;                              // mov     byte [edi], cl
L0026B1AB:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x000000F0;                                  // and     ecx, dword 0x000000F0
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B1BE;                       // jz      short L0026B1BE
    asm_shr(ecx, 0x04);                                 // shr     ecx, 0x04
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi+0x1)) = cl0;                          // mov     byte [edi+0x1], cl
L0026B1BE:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x00000F00;                                  // and     ecx, dword 0x00000F00
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B1D1;                       // jz      short L0026B1D1
    asm_shr(ecx, 0x08);                                 // shr     ecx, 0x08
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi+0x2)) = cl0;                          // mov     byte [edi+0x2], cl
L0026B1D1:
    ecx = ebx;                                          // mov     ecx, ebx
    ecx &= 0x0000F000;                                  // and     ecx, dword 0x0000F000
                                                        // test    ecx, ecx
    if (ecx == 0) goto L0026B1E4;                       // jz      short L0026B1E4
    asm_shr(ecx, 0x0C);                                 // shr     ecx, 0x0C
    cl0 = *((char*)(esi+ecx));                          // mov     cl, byte [esi+ecx]
    *((char*)(edi+0x3)) = cl0;                          // mov     byte [edi+0x3], cl
L0026B1E4:
    edi += edx;                                         // add     edi, edx
    asm_shr(ebx, 0x10);                                 // shr     ebx, 0x10
    eax -= 1;                                           // dec     eax
    if (eax != 0) goto L0026B19F;                       // jnz     short L0026B19F
    goto L0026B1FF;                                     // jmp     short L0026B1FF
L0026B1EE:
    eax = *((int*)(esp+0x4));                           // mov     eax, dword [esp+0x4]
    edx = *((int*)(esp+0x1C));                          // mov     edx, dword [esp+0x1C]
    eax <<= 0x02;                                       // shl     eax, 0x02
    edx += eax;                                         // add     edx, eax
    *((int*)(esp+0x1C)) = edx;                          // mov     dword [esp+0x1C], edx
L0026B1FF:
    ebx = *((int*)(esp+0x18));                          // mov     ebx, dword [esp+0x18]
    ecx = *((int*)(esp+0x1C));                          // mov     ecx, dword [esp+0x1C]
    esi = *((int*)(esp+0x10));                          // mov     esi, dword [esp+0x10]
    ebx += 0x04;                                        // add     ebx, byte 0x04
    ecx += 0x04;                                        // add     ecx, byte 0x04
    *((int*)(esp+0x18)) = ebx;                          // mov     dword [esp+0x18], ebx
    *((int*)(esp+0x1C)) = ecx;                          // mov     dword [esp+0x1C], ecx
                                                        // cmp     ecx, esi
    if ((uint)(ecx) < (uint)(esi)) goto outer_loop;     // jc      near outer_loop
L0026B221:
    eax = ebp;                                          // mov     eax, ebp
    //esp += 0x38;                                      // add     esp, byte 0x38
    //ebp = pop();                                      // pop     ebp
    //edi = pop();                                      // pop     edi
    //esi = pop();                                      // pop     esi
    //popra();
    return (uchar*)eax;                                 // ret     word 0x0004
}
