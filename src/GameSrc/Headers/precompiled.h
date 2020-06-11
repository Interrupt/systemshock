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
#include "pqueue.h"
#include "rect.h"
#include "slist.h"

#include "edms.h"
#include "edms_chk.h"

#include "fix.h"

#include "2dres.h"
// use explicit path for error.h include so it doesn't use system's <error.h>
#include "../../Libraries/H/error.h"
#include "keydefs.h"
#include "lg_types.h"
#include "lg.h"
#include "memall.h"
#include "tmpalloc.h"

#include "palette.h"

#include "res.h"
#include "lzw.h"

#include "rnd.h"

#include "lgsndx.h"

#include "event.h"
#include "hotkey.h"
#include "region.h"
#include "slab.h"
#include "vmouse.h"

#include "vox.h"

#ifdef __cplusplus
}
#endif
