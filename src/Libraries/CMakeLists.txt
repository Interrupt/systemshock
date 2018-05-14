enable_language(ASM)
set(can_use_assembler TRUE)

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
	2D/Source/GR/grilin.c
	2D/Source/GR/grmalloc.c
	2D/Source/GR/grnull.c
	2D/Source/GR/gruilin.c
	2D/Source/GR/grd.c
	2D/Source/Gen/gendisk.c
	2D/Source/Gen/gentm.c
	2D/Source/Gen/genwclin.c
	2D/Source/Gen/genuclin.c
	2D/Source/Gen/genrsd8.c
	2D/Source/Gen/genuvlin.c
	2D/Source/Gen/genhlin.c
	2D/Source/Gen/genrsdbm.c
	2D/Source/Gen/genuchr.c
	2D/Source/Gen/genvlin.c
	2D/Source/Gen/genvcply.c
	2D/Source/Gen/genstr.c
	2D/Source/Gen/genclin.c
	2D/Source/Gen/genfl8c.c
	2D/Source/Gen/genrsdtm.c
	2D/Source/Gen/genel.c
	2D/Source/Gen/genchr.c
	2D/Source/Gen/genwlin.c
	2D/Source/Gen/genpix.c
	2D/Source/Gen/genuhlin.c
	2D/Source/Gen/genustr.c
	2D/Source/Gen/genhfl8.c
	2D/Source/Gen/genov.c
	2D/Source/Gen/gente.c
	2D/Source/Gen/genlin.c
	2D/Source/Gen/gencwlin.c
	2D/Source/Gen/genchfl8.c
	2D/Source/Gen/gengfl8.c
	2D/Source/Gen/genbox.c
	2D/Source/Gen/genmono.c
	2D/Source/Gen/genulin.c
	2D/Source/Gen/gencnv.c
	2D/Source/Gen/genuslin.c
	2D/Source/Gen/genvpoly.c
	2D/Source/Gen/general.c
	2D/Source/Gen/gencirc.c
	2D/Source/Gen/genfl8.c
	2D/Source/Gen/genf24.c
	2D/Source/Gen/gentl8.c
	2D/Source/Gen/genrect.c
	2D/Source/Gen/genvrect.c
	2D/Source/Gen/genslin.c
	2D/Source/Flat8/fl8vlin.c
	2D/Source/Flat8/FL8OPL.c
	2D/Source/Flat8/fl8cply.c
	2D/Source/Flat8/fl8clin.c
	2D/Source/Flat8/fl8lp.c
	2D/Source/Flat8/fl8cft.h
	2D/Source/Flat8/fl8lf.c
	2D/Source/Flat8/fl8ll.c
	2D/Source/Flat8/fl8s.c
	2D/Source/Flat8/fl8hfl8.c
	2D/Source/Flat8/fl8pnt.c
	2D/Source/Flat8/fl8fl8m.c
	2D/Source/Flat8/fl8ltp.c
	2D/Source/Flat8/fl8fl8c.c
	2D/Source/Flat8/fl8w.c
	2D/Source/Flat8/fl8cop.c
	2D/Source/Flat8/fl8wlin.c
	2D/Source/Flat8/fl8lw.c
	2D/Source/Flat8/fl8pix.c
	2D/Source/Flat8/fl8row.c
	2D/Source/Flat8/fl8ft.c
	2D/Source/Flat8/fl8rsd8.c
	2D/Source/Flat8/Fl8F.c
	2D/Source/Flat8/fl8bldbl.c
	2D/Source/Flat8/fl8clear.c
	2D/Source/Flat8/fl8p.c
	2D/Source/Flat8/fl8wclin.c
	2D/Source/Flat8/fl8hlin.c
	2D/Source/Flat8/fl8bl.c
	2D/Source/Flat8/fl8gpix.c
	2D/Source/Flat8/fl8ns.c
	2D/Source/Flat8/fl8lnop.c
	2D/Source/Flat8/fl8dbl.c
	2D/Source/Flat8/fl8clin.h
	2D/Source/Flat8/fl8cnv.c
	2D/Source/Flat8/fl8slin.c
	2D/Source/Flat8/fl8lin.c
	2D/Source/Flat8/fl8tsmap.c
	2D/Source/Flat8/fl8ply.c
	2D/Source/Flat8/fl8sply.c
	2D/Source/Flat8/fl8rect.c
	2D/Source/Flat8/fl8gfl8.c
	2D/Source/Flat8/fl8tl8.c
	2D/Source/Flat8/fl8p24.c
	2D/Source/Flat8/fl8g24.c
	2D/Source/Flat8/fl8ctp.c
	2D/Source/Flat8/fl8mscl.c
	2D/Source/Flat8/fl8lop.c
	2D/Source/Flat8/fl8ntrp2.c
	2D/Source/Flat8/fl8sub.c
	2D/Source/Flat8/fl8tpl.c
	2D/Source/Flat8/fl8wclin.h
	2D/Source/Flat8/fl8nl.c
	2D/Source/Flat8/fl8fltr2.c
	2D/Source/Flat8/fl8mono.c
	2D/Source/Flat8/fl8fl8.c
	2D/Source/Flat8/fl8chfl8.c
	2D/Source/Clip/clpply.c
	2D/Source/Clip/clplin.c
	2D/Source/Clip/clpclin.c
	2D/Source/Clip/clpltab.c
	2D/Source/Clip/clpf24.c
	2D/Source/Clip/clplin2.c
	2D/Source/Clip/clppoly.c
	2D/Source/Clip/clpslin.c
	2D/Source/Clip/clprect.c
	2D/Source/Clip/clpmono.c
	2D/Source/RSD/rsdcvt.c
	2D/Source/RSD/RSDUnpack.c
)

set(GR_SRC
	2D/Source/GR/grd.c
	2D/Source/GR/grilin.c
	2D/Source/GR/grmalloc.c
	2D/Source/GR/grnull.c
	2D/Source/GR/gruilin.c
)


set(3D_SRC
	#3D/Source/3dTest.c
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
#	AFILE/Source/AddSoundToMovie.c
	AFILE/Source/afile.c
	AFILE/Source/amov.c
#	AFILE/Source/animconv.c
	AFILE/Source/aqtm.c
	AFILE/Source/compose.c
#	AFILE/Source/DumpHex.c
#	AFILE/Source/MovConv.c
#	AFILE/Source/MovConvDiv.c
#	AFILE/Source/movinfo.c
#	AFILE/Source/OldAnimConv.c
#	AFILE/Source/PalExtract.c
#	AFILE/Source/QTConv.c
#	AFILE/Source/quikconv.c
#	AFILE/Source/quikdump.c
#	AFILE/Source/quiktime.c
#	AFILE/Source/quikwrit.c
#	AFILE/Source/SubTitles.c
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
	#INPUT/Source/kbMac.c
	INPUT/Source/mouse.c
	INPUT/Source/sdl_events.c
)

set(LG_SRC
	LG/Source/dbg.c
	LG/Source/LGmem.c
	LG/Source/lgsprntf.c
	LG/Source/memall.c
	LG/Source/stack.c
	LG/Source/tmpalloc.c
)

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

set(FIXPP_SRC
	FIXPP/Source/fixpp.cpp
)

set(EDMS_SRC
	EDMS/Source/interfac.cc
	EDMS/Source/collide.cc
	EDMS/Source/intrsect.cc
	EDMS/Source/globals.cc
	EDMS/Source/phy_tool.cc
#	EDMS/Source/terrain.cc
	EDMS/Source/soliton.cc
	EDMS/Source/MODELS/robot.cc
#	EDMS/Source/MODELS/pelgud.cc
	EDMS/Source/MODELS/pelface.cc
	EDMS/Source/MODELS/marbface.cc
	EDMS/Source/MODELS/ftl.cc
	EDMS/Source/MODELS/pelvis.cc
	EDMS/Source/MODELS/marble.cc
	EDMS/Source/MODELS/d_frame.cc
	EDMS/Source/MODELS/d_f_face.cc
	EDMS/Source/MODELS/ff_face.cc
	EDMS/Source/MODELS/fodeath.cc
	EDMS/Source/MODELS/ftlface.cc
	EDMS/Source/MODELS/biped.cc
#	EDMS/Source/MODELS/b.cc
#	EDMS/Source/MODELS/ff_ftl.cc
	EDMS/Source/MODELS/cr_biped.cc
#	EDMS/Source/MODELS/old_ftl.cc
	EDMS/Source/MODELS/bip_skel.cc
	EDMS/Source/MODELS/bipface.cc
#	EDMS/Source/MODELS/bb.cc
	EDMS/Source/MODELS/death.cc
	EDMS/Source/MODELS/d_f_2.cc
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
	${CMAKE_SOURCE_DIR}/src/Libraries/FIXPP/Source
	${CMAKE_SOURCE_DIR}/src/Libraries/EDMS/Source
	${CMAKE_SOURCE_DIR}/src/Libraries/EDMS/Source/MODELS
)

add_library(2D_LIB ${2D_SRC})
add_library(GR_LIB ${GR_SRC})
add_library(3D_LIB ${3D_SRC})
add_library(AFILE_LIB ${AFILE_SRC})
add_library(PALETTE_LIB ${PALETTE_SRC})
add_library(DSTRUCT_LIB ${DSTRUCT_SRC})
add_library(FIX_LIB ${FIX_SRC})
add_library(INPUT_LIB ${INPUT_SRC})
add_library(LG_LIB ${LG_SRC})
add_library(RES_LIB ${RES_SRC})
add_library(RND_LIB ${RND_SRC})
#add_library(SND_LIB ${SND_SRC})
add_library(UI_LIB ${UI_SRC})
add_library(VOX_LIB ${VOX_SRC})
add_library(FIXPP_LIB ${FIXPP_SRC})
add_library(EDMS_LIB ${EDMS_SRC})

