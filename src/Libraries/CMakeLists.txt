

set(MAC_SRC
	../MacSrc/ShockBitmap.c
)

set(2D_SRC
	2D/Source/bit.c
	2D/Source/bitmap.c
	2D/Source/blend.c
	2D/Source/buffer.c
	2D/Source/canvas.c
	2D/Source/chain.c
	2D/Source/ChnFuncs.c
	2D/Source/chrsiz.c
	2D/Source/chrwid.c
	2D/Source/close.c
	2D/Source/cnvtab.c
	2D/Source/context.c
	2D/Source/detect.c
	2D/Source/devtab.c
	2D/Source/fcntab.c
	2D/Source/init.c
	2D/Source/invtab.c
	2D/Source/lintab.c
	2D/Source/mode.c
	2D/Source/pal.c
	2D/Source/permap.c
	2D/Source/persetup.c
	2D/Source/pertol.c
	2D/Source/pixfill.c
	2D/Source/rgb.c
	2D/Source/screen.c
	2D/Source/sscrn.c
	2D/Source/StateStk.c
	2D/Source/strnsiz.c
	2D/Source/strscl.c
	2D/Source/strsiz.c
	2D/Source/struscl.c
	2D/Source/strwid.c
	2D/Source/strwrap.c
	2D/Source/svgainit.c
	2D/Source/tempbm.c
	2D/Source/temptm.c
	2D/Source/tlucdat.c
	2D/Source/tluctab.c
	2D/Source/valloc.c
	2D/Source/vtab.c
	2D/Source/wirpoly.c
	2D/Source/MacDev.c
)	


set(3D_SRC
	3D/Source/3dTest.c
	3D/Source/alloc.c
	3D/Source/Bitmap.c
	3D/Source/clip.c
	3D/Source/detail.c
	3D/Source/fov.c
	3D/Source/GlobalV.c
	3D/Source/instance.c
	3D/Source/interp.c
	3D/Source/light.c
	3D/Source/matrix.c
	3D/Source/points.c
	3D/Source/polygon.c
	3D/Source/slew.c
	3D/Source/tmap.c
	3D/Source/vector.c
)

set(AFILE_SRC
	AFILE/Source/AddSoundToMovie.c
	AFILE/Source/afile.c
	AFILE/Source/amov.c
	AFILE/Source/animconv.c
	AFILE/Source/aqtm.c
	AFILE/Source/compose.c
	AFILE/Source/DumpHex.c
	AFILE/Source/MovConv.c
	AFILE/Source/MovConvDiv.c
	AFILE/Source/movinfo.c
	AFILE/Source/OldAnimConv.c
	AFILE/Source/PalExtract.c
	AFILE/Source/QTConv.c
	AFILE/Source/quikconv.c
	AFILE/Source/quikdump.c
	AFILE/Source/quiktime.c
	AFILE/Source/quikwrit.c
	AFILE/Source/SubTitles.c
)

set(DSTRUCT_SRC
	DSTRUCT/Source/array.c
	DSTRUCT/Source/hash.c
	DSTRUCT/Source/llist.c
	DSTRUCT/Source/lllist.c
	DSTRUCT/Source/pqueue.c
	DSTRUCT/Source/rect.c
)


set(FIX_SRC
	FIX/Source/atofix.c
	FIX/Source/f_exp.c
	FIX/Source/fix24.c
	FIX/Source/fix.c
	FIX/Source/fix_pow.c
	FIX/Source/fixsprnt.c
	FIX/Source/fix_sqrt.c
	FIX/Source/MakeTables.c
)

set(INPUT_SRC
	INPUT/Source/kbcook.c
	INPUT/Source/kbMac.c
	INPUT/Source/mouse.c
)

# Need cleanup
#set(LG_SRC
#	LG/Source/dbg.c
#	LG/Source/LGmem.c
#	LG/Source/lgsprntf.c
#	LG/Source/memall.c
#	LG/Source/stack.c
#	LG/Source/tmpalloc.c
#)

set(PALETTE_SRC
        PALETTE/Source/palette.c
)

set(RES_SRC
	RES/Source/lzw.c
	RES/Source/refacc.c
	RES/Source/resacc.c
	RES/Source/resbuild.c
	RES/Source/res.c
	RES/Source/resfile.c
	RES/Source/resload.c
	RES/Source/resmake.c
	RES/Source/restypes.c
)

set(RND_SRC
	RND/Source/rnd.c
)

set(SND_SRC
	SND/Source/dig_init.c
	SND/Source/dig_ops.c
	SND/Source/master.c
	SND/Source/mid_init.c
	SND/Source/mid_ops.c
	SND/Source/snd_util.c
)

set(UI_SRC
	UI/Source/butarray.c
	UI/Source/curdrw.c
	UI/Source/cursors.c
	UI/Source/event.c
	UI/Source/gadgets.c
	UI/Source/hotkey.c
	UI/Source/menu.c
	UI/Source/monofont.c
	UI/Source/plain.c
	UI/Source/pushbutt.c
	UI/Source/qboxgadg.c
	UI/Source/realfont.c
	UI/Source/region.c
	UI/Source/slab.c
	UI/Source/slider.c
	UI/Source/textgadg.c
	UI/Source/texttool.c
	UI/Source/tngbarry.c
	UI/Source/tng.c
	UI/Source/tngfont.c
	UI/Source/tngmenu.c
	UI/Source/tngplain.c
	UI/Source/tngpushb.c
	UI/Source/tngqbox.c
	UI/Source/tngslidr.c
	UI/Source/tngtemp.c
	UI/Source/tngtextg.c
	UI/Source/uires.c
	UI/Source/vmouse.c
)


set(VOX_SRC
	VOX/Source/vox2d.c
	VOX/Source/vox3d.c
	VOX/Source/voxinit.c
)

include_directories(
	${CMAKE_SOURCE_DIR}/src/MacSrc
	${CMAKE_SOURCE_DIR}/src/Libraries/2D/Source/GR
	${CMAKE_SOURCE_DIR}/src/Libraries/2D/Source/Clip
	${CMAKE_SOURCE_DIR}/src/Libraries/2D/Source/Flat8
	${CMAKE_SOURCE_DIR}/src/Libraries/2D/Source/Gen
	${CMAKE_SOURCE_DIR}/src/Libraries/2D/Source/RSD
	${CMAKE_SOURCE_DIR}/src/Libraries/2D/Source
	${CMAKE_SOURCE_DIR}/src/Libraries/3D/Source
	${CMAKE_SOURCE_DIR}/src/Libraries/DSTRUCT/Source
	${CMAKE_SOURCE_DIR}/src/Libraries/H
	${CMAKE_SOURCE_DIR}/src/Libraries/INPUT/Source
	${CMAKE_SOURCE_DIR}/src/Libraries/FIX/Source
    ${CMAKE_SOURCE_DIR}/src/Libraries/LG/Source
	${CMAKE_SOURCE_DIR}/src/Libraries/RES/Source
)

add_library(MAC_LIB ${MAC_SRC})
add_library(2D_LIB ${2D_SRC})
add_library(3D_LIB ${3D_SRC})
#add_library(AFILE_LIB ${AFILE_SRC})
add_library(PALLETE_LIB ${PALETTE_SRC})
add_library(DSTRUCT_LIB ${DSTRUCT_SRC})
add_library(FIX_LIB ${FIX_SRC})
add_library(INPUT_LIB ${INPUT_SRC})
#add_library(LG_LIB ${LG_SRC})
add_library(RES_LIB ${RES_SRC})
add_library(RND_LIB ${RND_SRC})
#add_library(SND_LIB ${SND_SRC})
add_library(UI_LIB ${UI_SRC})
add_library(VOX_LIB ${VOX_SRC})

