project(SystemShock)

cmake_minimum_required(VERSION 3.1)

set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS ON)

# Required for stdbool.h
set(CMAKE_C_STANDARD 99)

# hide Windows console in distributable packages
if(DEFINED ENV{APPVEYOR})
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-O3 -D__STDC_LIMIT_MACROS")
endif()

if(NOT DEFINED ENV{APPVEYOR})
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-D__STDC_LIMIT_MACROS")
endif()

option(ENABLE_EXAMPLES "Enable example applications" OFF)
option(ENABLE_DEBUG_BLIT "Enable debugging blitter" OFF)

# HAAAAX!!
add_definitions(-DSVGA_SUPPORT)

if(ENABLE_DEBUG_BLIT)
	add_definitions(-DDEBUGGING_BLIT)
endif()

add_compile_options(-fsigned-char -fno-strict-aliasing -Werror=implicit-function-declaration)

# Find OpenGL
find_package(OpenGL)
if(OPENGL_FOUND)
	message(STATUS "OpenGL found")
	add_definitions(-DUSE_OPENGL)
ENDIF(OPENGL_FOUND)

# set up SDL

find_package (SDL2 REQUIRED)

# and SDL mixer

find_library(SDL2_MIXER_LIBRARY SDL2_mixer PATHS ${SDL2_MIXER_DIR}/lib)
set(SDL2_MIXER_LIBRARIES ${SDL2_MIXER_LIBRARY})

add_definitions(-DUSE_SDL_MIXER=1)


# add MinGW library if necessary

if(MINGW)
	set(SDL2_LIBRARIES "mingw32 ${SDL2_LIBRARIES}")
	set(SDL2_MIXER_LIBRARIES "mingw32 ${SDL2_MIXER_LIBRARIES}")
endif(MINGW)


include_directories(
	${SDL2_INCLUDE_DIRS}
	${SDL2_MIXER_INCLUDE_DIRS}
)

message(STATUS "Found SDL2 Include:${SDL2_INCLUDE_DIRS} Library:${SDL2_LIBRARIES}")
message(STATUS "Found SDL2_Mixer Include:${SDL2_MIXER_INCLUDE_DIRS} Library:${SDL2_MIXER_LIBRARIES}")

# Find FluidSynth
find_library(FLUIDSYNTH_LIBRARY fluidsynth PATHS build_ext/fluidsynth-lite/src)

if(NOT WIN32)
  # Find ALSA for Linux native MIDI
  # NOTE: this seems to require having 64-bit dev pacakges installed when building
  #  on 64-bit OS, even when building a 32-bit binary
  find_package(ALSA)
  if(ALSA_FOUND)
      message(STATUS "ALSA found")
      include_directories(${ALSA_INCLUDE_DIRS})
      add_definitions(-DUSE_ALSA=1)
  endif(ALSA_FOUND)
endif(NOT WIN32)

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
	src/MacSrc/OpenGL.cc
	src/MacSrc/Xmi.c
	src/MusicSrc/MusicDevice.c
)

set(GAME_SRC
	src/GameSrc/ai.c
	src/GameSrc/airupt.c
	src/GameSrc/amap.c
	src/GameSrc/amaploop.c
	src/GameSrc/ammomfd.c
	src/GameSrc/anim.c
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
	src/GameSrc/cutsloop.c
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
	src/Libraries/AFILE/Source
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
	src/Libraries/LG/Source/LOG/src
	src/Libraries/adlmidi/include
	src/GameSrc/Headers
	src/MacSrc
	src/MusicSrc
)

if(ENABLE_EXAMPLES)

add_executable(playmov
	src/Libraries/AFILE/Tests/playmov.c
)
add_executable(movinfo
	src/Libraries/AFILE/Tests/movinfo.c
)

target_link_libraries(playmov
	AFILE_LIB
	FIX_LIB
	${SDL2_LIBRARIES}
	${SDL2_MIXER_LIBRARIES}
)

target_link_libraries(movinfo
	AFILE_LIB
	FIX_LIB
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
	ADLMIDI_LIB
	${SDL2_LIBRARIES}
	${SDL2_MIXER_LIBRARIES}
	${OPENGL_LIBRARIES}
	${ALSA_LIBRARIES}
)

if(FLUIDSYNTH_LIBRARY)
  message(STATUS "Fluidsynth found")
  add_definitions("-DUSE_FLUIDSYNTH=1")
  target_link_libraries(systemshock "${FLUIDSYNTH_LIBRARY}")
  include_directories(${CMAKE_SOURCE_DIR}/build_ext/fluidsynth-lite/include)
endif()

if (WIN32)
        include_directories(${CMAKE_SOURCE_DIR}/build_ext/built_glew/include)
        target_link_libraries(systemshock ${CMAKE_SOURCE_DIR}/build_ext/built_glew/lib/libglew32.dll.a winmm)
endif(WIN32)

# Turn on address sanitizing if wanted
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
add_sanitizers(systemshock)
