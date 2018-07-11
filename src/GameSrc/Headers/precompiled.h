#pragma pack(2)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>

#include "2d.h"

#include "3d.h"
#include "3dinterp.h"

#include "array.h"
#include "hash.h"
#include "llist.h"
#include "pqueue.h"
#include "rect.h"
#include "slist.h"

#include "edms.h"
#include "edms_chk.h"

#include "fix.h"

//#include "fixpp.h"

#include "2dres.h"
//#include "base.h"
// use explicit path for error.h include so it doesn't use system's <error.h>
#include "../../Libraries/H/error.h"
#include "keydefs.h"
#include "lg_types.h"
#include "lgsprntf.h"

//#include "kb.h"
//#include "kbcook.h"
//#include "kbglob.h"
//#include "mouse.h"
//#include "mousevel.h"

#include "dbg.h"
#include "lg.h"
#include "memall.h"
#include "tmpalloc.h"

#include "palette.h"

#include "res.h"
#include "lzw.h"

#include "rnd.h"

// NO SOUND YET!
#include "lgsndx.h"
//#include "digi.h"
//#include "midi.h"

//#include "cursors.h"
//#include "curtyp.h"
//#include "curdat.h"
#include "butarray.h"
#include "event.h"
#include "gadgets.h"
#include "hotkey.h"
#include "menu.h"
#include "plain.h"
#include "pushbutt.h"
#include "qboxgadg.h"
#include "region.h"
#include "slab.h"
#include "slider.h"
#include "vmouse.h"

#include "vox.h"

#ifdef __cplusplus
}
#endif
