/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.

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
/*
 * $Source: r:/prj/cit/src/RCS/citres.c $
 * $Revision: 1.20 $
 * $Author: xemu $
 * $Date: 1994/11/07 13:22:58 $
 *
 */

#include <string.h>

#include "citres.h"
#include "criterr.h"
#include "gr2ss.h"
#include "statics.h"

// Internal Prototypes
errtype master_load_bitmap_from_res(grs_bitmap *bmp, Id id_num, int i, LGRect *anchor, uchar *p);

grs_bitmap *lock_bitmap_from_ref_anchor(Ref r, LGRect *anchor) {
    FrameDesc *f;
    if (r == 0)
        return (NULL);
    f = RefLock(r);
    if (f == NULL) {
        //      Warning(("Could not lock bitmap %d!",r));
        return (NULL);
    }
    f->bm.bits = (uchar *)(f + 1);
    if (anchor != NULL)
        *anchor = f->anchorArea;
    //   DBG((DSRC_GFX_Anim),
    //   {
    //      ss_bitmap(&(f->bm),0,0);
    //   });
    return (&(f->bm));
}

grs_bitmap *get_bitmap_from_ref_anchor(Ref r, LGRect *anchor) {
    grs_bitmap *retval = lock_bitmap_from_ref_anchor(r, anchor);
    RefUnlock(r);
    return retval;
}

#pragma mark -

#pragma scheduling off
#pragma global_optimizer off

errtype master_load_bitmap_from_res(grs_bitmap *bmp, Id id_num, int i, LGRect *anchor, uchar *p) {
    extern int memcount;
    Ref rid = MKREF(id_num, i);
    FrameDesc *f = RefGet(rid);

    if (f == NULL) {
        //      Warning(("Could not load bitmap from resource #%d!\n",id_num));
        printf("Could not load bitmap from resource #%d!\n", id_num);
        return (ERR_FREAD);
    }

    if (p == NULL) {
	// Caller wants us to allocate a framebuffer.
        p = malloc(f->bm.w * f->bm.h);
    }

    if (anchor != NULL)
        *anchor = f->anchorArea;

    // Copy the bitmap structure across.
    if (bmp == NULL)
        DEBUG("%s: Trying to assign to a null bmp pointer!", __FUNCTION__);
    *bmp = f->bm;

    // Copy the bits.
    memcount += f->bm.w * f->bm.h; // FIXME is this needed any more?
    if (f->bm.type == BMT_RSD8) {
	gr_rsd8_convert(&f->bm, bmp);
	// gr_rsd8_convert uses its own buffer, so copy it back.
	memcpy(p, bmp->bits, f->bm.w * f->bm.h);
    } else {
	memcpy(p, f->bm.bits, f->bm.w * f->bm.h);
    }
    bmp->bits = p;

    return (OK);
}

#pragma scheduling reset
#pragma global_optimizer reset

#pragma mark -

errtype load_bitmap_from_res(grs_bitmap *bmp, Id id_num, int i, uchar transp, LGRect *anchor) {
    return master_load_bitmap_from_res(bmp, id_num, i, anchor, NULL);
}

errtype load_res_bitmap(grs_bitmap *bmp, Ref rid, uchar alloc) {
    errtype retval;

    // printf("load_res_bitmap %x : %x\n", REFID(rid), REFINDEX(rid));
    retval = master_load_bitmap_from_res(bmp, REFID(rid), REFINDEX(rid), NULL, (alloc) ? NULL : bmp->bits);

    return (retval);
}

#ifdef SIMPLER_NONEXTRACTING_WAY
errtype load_res_bitmap(grs_bitmap *bmp, Ref rid, uchar alloc) {
    errtype retval = OK;
    char *bits = bmp->bits;
    FrameDesc *f;
    int sz;
    extern int memcount;

    f = RefLock(rid);
    sz = f->bm.w * f->bm.h;
    if (alloc) {
        bits = malloc(sz);
        if (bits == NULL) {
            retval = ERR_NOMEM;
            goto out;
        }
    }
    LG_memcpy(bits, (char *)(f + 1), sz);
    *bmp = f->bm;
    bmp->bits = bits;
out:
    RefUnlock(rid);
    return retval;
}
#endif

errtype simple_load_res_bitmap(grs_bitmap *bmp, Ref rid) { return load_res_bitmap(bmp, rid, TRUE); }

#pragma mark -

errtype load_res_bitmap_cursor(LGCursor *c, grs_bitmap *bmp, Ref rid, uchar alloc) {
    errtype retval = OK;
    LGRect anchor;

#ifdef SVGA_SUPPORT
    short w, h;
    short temp;
    uchar *bits;
    grs_bitmap temp_bmp;
    grs_canvas temp_canv;
    uchar old_over = gr2ss_override;
    ss_set_hack_mode(2, &temp);

    gr2ss_override = OVERRIDE_ALL;
    master_load_bitmap_from_res(&temp_bmp, REFID(rid), REFINDEX(rid), &anchor, NULL);
    w = temp_bmp.w;
    h = temp_bmp.h;
    ss_point_convert(&w, &h, FALSE);
    if (alloc)
        bits = (uchar *)malloc(sizeof(char) * w * h);
    else
        bits = bmp->bits;
    if (temp_bmp.bits == NULL)
        critical_error(CRITERR_MEM | 5);
    gr_init_bm(bmp, bits, BMT_FLAT8, BMF_TRANS, w, h);
    gr_make_canvas(bmp, &temp_canv);
    gr_push_canvas(&temp_canv);
    gr_clear(0);
    ss_bitmap(&temp_bmp, 0, 0);
    free(temp_bmp.bits);
    if (convert_use_mode) {
        anchor.ul.x = (SCONV_X(anchor.ul.x) + SCONV_X(anchor.ul.x + 1)) / 2;
        anchor.ul.y = (SCONV_Y(anchor.ul.y) + SCONV_Y(anchor.ul.y + 1)) / 2;
    }
    //   gr_set_pixel(34,anchor.ul.x,anchor.ul.y);  // test test test
    gr_pop_canvas();
    retval = uiMakeBitmapCursor(c, bmp, anchor.ul);
    ss_set_hack_mode(0, &temp);
    gr2ss_override = old_over;
#else
    retval = master_load_bitmap_from_res(bmp, REFID(rid), REFINDEX(rid),
                                         &anchor, (alloc) ? NULL : bmp->bits);
    if (retval == OK) {
        retval = uiMakeBitmapCursor(c, bmp, anchor.ul);
    }
#endif
    return retval;
}

errtype simple_load_res_bitmap_cursor(LGCursor *c, grs_bitmap *bmp, Ref rid) {
    return load_res_bitmap_cursor(c, bmp, rid, TRUE);
}

errtype load_hires_bitmap_cursor(LGCursor *c, grs_bitmap *bmp, Ref rid, uchar alloc) {
    errtype retval = OK;
    LGRect anchor;

    retval = master_load_bitmap_from_res(bmp, REFID(rid), REFINDEX(rid),
                                         &anchor, (alloc) ? NULL : bmp->bits);
    if (retval == OK) {
        retval = uiMakeBitmapCursor(c, bmp, anchor.ul);
    }

    return retval;
}

/*
void *CitMalloc(int n)
{
   return(Malloc(n));
}

void CitFree(void *p)
{
   Free(p);
}

*/
