
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# specify the cross compiler
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

set(CMAKE_C_FLAGS -m32)
set(CMAKE_CXX_FLAGS -m32)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-m32")

# where is the target environment
set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32 /usr/i686-w64-mingw32/sys-root/mingw)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# CMake determines how to examine dependencies based on the *host* system, leading to
# a `file unknown error` unless the target platform is explicitly specified.
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")

set(TOOLCHAIN_DLL_DEPENDENCIES libgcc_s_dw2-1.dll libstdc++-6.dll)
