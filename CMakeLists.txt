project(SystemShock)

cmake_minimum_required(VERSION 3.1)

# Required for stdbool.h
set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS -m32)
set(CMAKE_CXX_FLAGS -m32)

# HAAAAX!!
add_definitions(-Dpowerc -DSVGA_SUPPORT)

add_compile_options(-fno-strict-aliasing -Werror=implicit-function-declaration)

# Find SDL
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# HACK: make carbon an empty list or sth
set(CARBON  )

# for now I'll put a Carbon/Carbon.h into src/stubs/
# that defines Mac-specific types and provides stubs for used functions
# like GetResource()
include_directories(src/stubs/)

add_subdirectory(src/Libraries/)

set(MAC_SRC
	src/MacSrc/Stub.c
	src/MacSrc/ShockBitmap.c
	src/MacSrc/MoviePlay.c
	src/MacSrc/InitMac.c
	src/MacSrc/Shock.c
	src/MacSrc/Prefs.c
	src/MacSrc/MacTune.c
#	src/MacSrc/Tests.c
)

if(NOT APPLE)
	set(MAC_SRC ${MAC_SRC} src/stubs/Carbon/carbon_stubs.c)
endif()

set(GAME_SRC
	src/GameSrc/ai.c
	src/GameSrc/airupt.c
	src/GameSrc/amap.c
	src/GameSrc/amaploop.c
	src/GameSrc/ammomfd.c
	src/GameSrc/audiolog.c
	src/GameSrc/automap.c
	src/GameSrc/bark.c
	src/GameSrc/biohelp.c
	src/GameSrc/cardmfd.c
	src/GameSrc/citres.c
	src/GameSrc/combat.c
	src/GameSrc/cone.c
	src/GameSrc/criterr.c
	src/GameSrc/cyber.c
	src/GameSrc/cybermfd.c
	src/GameSrc/cybmem.c
	src/GameSrc/cybrnd.c
	src/GameSrc/damage.c
	src/GameSrc/digifx.c
	src/GameSrc/drugs.c
	src/GameSrc/effect.c
	src/GameSrc/email.c
	src/GameSrc/faceobj.c
	src/GameSrc/fixtrmfd.c
	src/GameSrc/frcamera.c
	src/GameSrc/frclip.c
	src/GameSrc/frcompil.c
	src/GameSrc/frmain.c
	src/GameSrc/frobj.c
	src/GameSrc/froslew.c
	src/GameSrc/frpipe.c
	src/GameSrc/frpts.c
	src/GameSrc/frsetup.c
	src/GameSrc/frtables.c
	src/GameSrc/frterr.c
	src/GameSrc/frutil.c
	src/GameSrc/FrUtils.c
	src/GameSrc/fullamap.c
	src/GameSrc/fullscrn.c
	src/GameSrc/gameloop.c
	src/GameSrc/gameobj.c
	src/GameSrc/gamesort.c
	src/GameSrc/gamestrn.c
	src/GameSrc/gamesys.c
	src/GameSrc/gametime.c
	src/GameSrc/gamewrap.c
	src/GameSrc/gearmfd.c
	src/GameSrc/gr2ss.c
	src/GameSrc/grenades.c
	src/GameSrc/hand.c
	src/GameSrc/hflip.c
	src/GameSrc/hkeyfunc.c
	src/GameSrc/hud.c
	src/GameSrc/hudobj.c
	src/GameSrc/init.c
	src/GameSrc/input.c
	src/GameSrc/invent.c
	src/GameSrc/leanmetr.c
	src/GameSrc/mainloop.c
	src/GameSrc/map.c
	src/GameSrc/mfdfunc.c
	src/GameSrc/mfdgadg.c
	src/GameSrc/mfdgames.c
	src/GameSrc/mfdgump.c
	src/GameSrc/mfdpanel.c
	src/GameSrc/minimax.c
	src/GameSrc/mlimbs.c
	src/GameSrc/movekeys.c
	src/GameSrc/musicai.c
	src/GameSrc/newai.c
	src/GameSrc/newmfd.c
	src/GameSrc/objapp.c
	src/GameSrc/objects.c
	src/GameSrc/objload.c
	src/GameSrc/objprop.c
	src/GameSrc/objsim.c
	src/GameSrc/objuse.c
	src/GameSrc/olh.c
	src/GameSrc/olhscan.c
	src/GameSrc/palfx.c
	src/GameSrc/pathfind.c
	src/GameSrc/physics.c
	src/GameSrc/player.c
	src/GameSrc/plotware.c
	src/GameSrc/popups.c
	src/GameSrc/render.c
	src/GameSrc/rendtool.c
	src/GameSrc/saveload.c
	src/GameSrc/schedule.c
	src/GameSrc/screen.c
	src/GameSrc/setup.c
	src/GameSrc/shodan.c
	src/GameSrc/sideicon.c
	src/GameSrc/sndcall.c
	src/GameSrc/star.c
	src/GameSrc/statics.c
	src/GameSrc/status.c
	src/GameSrc/target.c
	src/GameSrc/textmaps.c
	src/GameSrc/tfdirect.c
	src/GameSrc/tfutil.c
	src/GameSrc/tools.c
	src/GameSrc/trigger.c
	src/GameSrc/view360.c
	src/GameSrc/viewhelp.c
	src/GameSrc/vitals.c
	src/GameSrc/vmail.c
	src/GameSrc/wares.c
	src/GameSrc/weapons.c
	src/GameSrc/wrapper.c
	src/GameSrc/gamerend.c
)

include_directories(
	src/Libraries/2D/Source
	src/Libraries/3D/Source
	src/Libraries/DSTRUCT/Source
	src/Libraries/EDMS/Source
	src/Libraries/FIXPP/Source
	src/Libraries/INPUT/Source
	src/Libraries/PALETTE/Source
	src/Libraries/RES/Source
	src/Libraries/UI/Source
	src/Libraries/RND/Source
	src/Libraries/VOX/Source
	src/Libraries/FIX/Source
	src/Libraries/SND/Source
	src/Libraries/H
	src/Libraries/LG/Source
	src/GameSrc/Headers
	src/MacSrc
)

add_executable(TestSimpleMain
	src/Libraries/2D/TestSource/SimpleMain.c
)

target_link_libraries(TestSimpleMain
	2D_LIB
	GR_LIB
	3D_LIB
	RES_LIB
	FIX_LIB
	LG_LIB
	${SDL2_LIBRARIES}
	${CARBON}
)

add_executable(BoxTest
	src/Libraries/3D/Tests/BoxTest.c
)

target_link_libraries(BoxTest
	2D_LIB
	GR_LIB
	3D_LIB
	RES_LIB
	FIX_LIB
	LG_LIB
	${SDL2_LIBRARIES}
	${CARBON}
)

add_executable(BitmapTest
	src/Libraries/3D/Tests/BitmapTest.c
)

target_link_libraries(BitmapTest
	2D_LIB
	GR_LIB
	3D_LIB
	RES_LIB
	FIX_LIB
	LG_LIB
	${SDL2_LIBRARIES}
	${CARBON}
)

add_executable(FixTest
	src/Libraries/FIX/Tests/FixTest/fixtest.c
)

target_link_libraries(FixTest
	FIX_LIB
	${CARBON}
	LG_LIB
)

add_executable(FixPPTest
	src/Libraries/FIXPP/Tests/FixppTest.cc
)

target_link_libraries(FixPPTest
	FIXPP_LIB
	${CARBON}
	LG_LIB
)

if(APPLE)
# Find Carbon
find_library(CARBON NAMES Carbon)
endif()

# Include magic header file, set struct packing size
add_definitions(
	-include precompiled.h
)

add_executable(systemshock
	${MAC_SRC}
)

add_library(GAME_LIB ${GAME_SRC})

target_link_libraries(systemshock
	2D_LIB
	LG_LIB
	GR_LIB
	3D_LIB
	RND_LIB
	AFILE_LIB
	DSTRUCT_LIB
	FIX_LIB
	INPUT_LIB
	PALETTE_LIB
	RES_LIB
#	SND_LIB
	UI_LIB
	VOX_LIB
	EDMS_LIB
	FIXPP_LIB
	GAME_LIB
	${CARBON}
	${SDL2_LIBRARIES}
)
