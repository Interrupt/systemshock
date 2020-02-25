cmake_minimum_required(VERSION 3.11)

project(shockolate VERSION 0.7.8)

# Prefer new GLVND library interface
if(POLICY CMP0072)
	cmake_policy(SET CMP0072 NEW)
endif()

#set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
#set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS ON)
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS OFF)
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)

# Required for stdbool.h
set(CMAKE_C_STANDARD 99)
# For nullptr in C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FIXME why this is required?
add_definitions(-D__STDC_LIMIT_MACROS)

option(ENABLE_EXAMPLES "Enable example applications" OFF)
option(ENABLE_DEBUG_BLIT "Enable debugging blitter" OFF)
option(ENABLE_OPENGL "Enable OpenGL support" ON)
option(ENABLE_SOUND "Enable sound support (requires SDL2_mixer)" ON)
# Tristate Fluidsynth
set(ENABLE_FLUIDSYNTH "LITE" CACHE STRING "Enable FluidSynth MIDI support (ON/LITE/OFF, default LITE)")
set_property(CACHE ENABLE_FLUIDSYNTH PROPERTY STRINGS "ON" "LITE" "OFF")

# HAAAAX!!
add_definitions(-DSVGA_SUPPORT)

if(ENABLE_DEBUG_BLIT)
	add_definitions(-DDEBUGGING_BLIT)
endif()

add_compile_options(-fsigned-char -fno-strict-aliasing)

# Find OpenGL
if(ENABLE_OPENGL)
	find_package(OpenGL REQUIRED)
	add_definitions(-DUSE_OPENGL)
	list(APPEND SHOCK_INCLUDES ${OPENGL_INCLUDE_DIRS})
	list(APPEND SHOCK_LIBRARIES ${OPENGL_LIBRARIES})
	if(MINGW)
		set(GLEW_USE_STATIC_LIBS 1)
		find_package(GLEW REQUIRED)
		list(APPEND SHOCK_INCLUDES ${GLEW_INCLUDE_DIRS})
		list(APPEND SHOCK_LIBRARIES GLEW::GLEW)
	endif(MINGW)
endif(ENABLE_OPENGL)

find_package(SDL2 REQUIRED)
list(APPEND SHOCK_INCLUDES ${SDL2_INCLUDE_DIRS})
list(APPEND SHOCK_LIBRARIES ${SDL2_LIBRARIES})

if(ENABLE_SOUND)
	# FIXME applies only for *nix systems
	find_package(PkgConfig)
	pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer>=2.0.4)
	add_definitions(-DUSE_SDL_MIXER)
	list(APPEND SHOCK_INCLUDES ${SDL2_MIXER_INCLUDE_DIRS})
	list(APPEND SHOCK_LIBRARIES ${SDL2_MIXER_LIBRARIES})
endif(ENABLE_SOUND)

if(ENABLE_FLUIDSYNTH STREQUAL "LITE")
	include(FetchContent)
	FetchContent_Declare(
		fluidsynth-lite
		# For time being https://github.com/Doom64/fluidsynth-lite/pull/10
		# https://github.com/Doom64/fluidsynth-lite
		GIT_REPOSITORY https://github.com/winterheart/fluidsynth-lite.git
		GIT_SHALLOW	ON
		GIT_TAG	a414e3055a652b5c8bb34b6e5b43e537468ccde6 # 26.01.2020
	)
	FetchContent_GetProperties(fluidsynth-lite)
	if(NOT fluidsynth-lite_POPULATED)
		message(STATUS "Populating fluidsynth-lite...")
		FetchContent_Populate(fluidsynth-lite)
		add_subdirectory(${fluidsynth-lite_SOURCE_DIR} ${fluidsynth-lite_BINARY_DIR})
		set_property(TARGET libfluidsynth PROPERTY C_STANDARD 11)
		message(STATUS "Done.")
	endif()
	include_directories(BEFORE SYSTEM ${fluidsynth-lite_SOURCE_DIR}/include ${fluidsynth-lite_BINARY_DIR}/include)
	add_definitions("-DUSE_FLUIDSYNTH")
	list(APPEND SHOCK_LIBRARIES libfluidsynth)
elseif(ENABLE_FLUIDSYNTH)
	# Use bulky system FluidSynth
	find_package(PkgConfig)
	pkg_check_modules(FLUIDSYNTH REQUIRED fluidsynth)
	list(APPEND SHOCK_INCLUDES ${FLUIDSYNTH_INCLUDE_DIRS})
	list(APPEND SHOCK_LIBRARIES ${FLUIDSYNTH_LIBRARIES})
	add_definitions("-DUSE_FLUIDSYNTH")
endif(ENABLE_FLUIDSYNTH STREQUAL "LITE")

if(NOT WIN32)
	# Find ALSA for Linux native MIDI
	# NOTE: this seems to require having 64-bit dev packages installed when building
	#  on 64-bit OS, even when building a 32-bit binary
	find_package(ALSA)
	if(ALSA_FOUND)
		message(STATUS "ALSA found")
		list(APPEND SHOCK_INCLUDES ${ALSA_INCLUDE_DIRS})
		list(APPEND SHOCK_LIBRARIES ${ALSA_LIBRARIES})
		add_definitions(-DUSE_ALSA)
	endif(ALSA_FOUND)
endif(NOT WIN32)

include_directories(
	${SHOCK_INCLUDES}
)

# Generate version based on project version
set(PROJECT_REVERSION_STRING "")
find_package(Git)
if(GIT_FOUND)
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE git_describe_out
		ERROR_VARIABLE git_describe_error
		RESULT_VARIABLE git_describe_result
		OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	set(PROJECT_REVERSION_STRING "-g${git_describe_out}")
endif(GIT_FOUND)

message(STATUS "Version is ${PROJECT_VERSION}${PROJECT_REVERSION_STRING}")
configure_file("${CMAKE_SOURCE_DIR}/src/GameSrc/Headers/shockolate_version.h.in"
	"${CMAKE_BINARY_DIR}/src/GameSrc/Headers/shockolate_version.h" )

# Sources configuration
add_subdirectory(src/Libraries/)

set(MAC_SRC
	src/MacSrc/ShockBitmap.c
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
	src/GameSrc/archiveformat.c
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
	src/GameSrc/tickcount.c
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
	${CMAKE_BINARY_DIR}/src/GameSrc/Headers
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

# MINGW additional linker options
if(MINGW)
	set(WINDOWS_LIBRARIES mingw32 -mwindows winmm)
endif(MINGW)

target_link_libraries(systemshock
	${WINDOWS_LIBRARIES} # Set it before any linker options! Beware WinMain@16 error!!
	${SHOCK_LIBRARIES}
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
)

# Turn on address sanitizing if wanted
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
add_sanitizers(systemshock)
