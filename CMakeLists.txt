project(SystemShock)

cmake_minimum_required(VERSION 3.1)

# Required for stdbool.h
set(CMAKE_C_STANDARD 99)

# This was useful when figuring out what's going wrong
# set(CMAKE_VERBOSE_MAKEFILE true)

option(ENABLE_EXAMPLES "Enable example applications" OFF)

# TODO compile the main SDL and make a similar thing for it
# right now my SDL installation is found via ${SDL2_DIR}
set(VC_LIB_PATH_SUFFIX VisualC/Win32/Debug)
set(CMAKE_FIND_LIBRARY_PREFIXES "")
set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)

set(SDL2_PATH "build_ext/SDL2-2.0.8/")
set(SDL2_MIXER_PATH "build_ext/SDL_mixer")

# HAAAAX!!
add_definitions(-Dpowerc -DSVGA_SUPPORT -DSDL_MAIN_HANDLED)

# Find SDL, trying the easy way and then the hard way
find_package(SDL2)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/findsdl2-cmake" ${CMAKE_MODULE_PATH})
IF(NOT SDL2_LIBRARIES)
	message(STATUS "Trying SDL2 search fallback")
	find_package(SDL2 REQUIRED)
	set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
	set(SDL2_LIBRARIES ${SDL2_LIBRARY})
ENDIF(NOT SDL2_LIBRARIES)

# Try to find SDL_mixer, for sound
find_package(SDL2_mixer)

include_directories(
	${SDL2_INCLUDE_DIRS}
	${SDL2_MIXER_INCLUDE_DIRS}
)

message(STATUS "Found SDL2 Include:${SDL2_INCLUDE_DIRS} Library:${SDL2_LIBRARIES}")
message(STATUS "Found SDL2_Mixer Include:${SDL2_MIXER_INCLUDE_DIRS} Library:${SDL2_MIXER_LIBRARIES}")

# not actually sure if this does anything under MSVC...
set(CMAKE_C_FLAGS "/I${SDL2_INCLUDE_DIRS}")


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
	src/MacSrc/SDLSound.c
	src/MacSrc/Modding.c
)

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
	src/GameSrc/mouselook.c
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

if(ENABLE_EXAMPLES)
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
)

add_executable(FixTest
	src/Libraries/FIX/Tests/FixTest/fixtest.c
)

target_link_libraries(FixTest
	FIX_LIB
	LG_LIB
)

add_executable(FixPPTest
	src/Libraries/FIXPP/Tests/FixppTest.cc
)

target_link_libraries(FixPPTest
	FIX_LIB
	FIXPP_LIB
	LG_LIB
)
endif()

# Include magic header file, set struct packing size
# Couldn't figure out a way to make this work, ended up doing it in the GUI instead
add_definitions(
	#/FI"src/GameSrc/Headers/precompiled.h"
)

add_executable(systemshock
	${MAC_SRC}
)

add_library(GAME_LIB ${GAME_SRC})

target_link_libraries(systemshock
	GAME_LIB
	UI_LIB
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
	VOX_LIB
	EDMS_LIB
	FIXPP_LIB
	${SDL2_LIBRARIES}
	${SDL2_MIXER_LIBRARIES}
)

# Turn on address sanitizing if wanted
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
add_sanitizers(systemshock)


