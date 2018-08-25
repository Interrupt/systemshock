# We probably need to tell OSX which SDL version to use, or it will try to use the system install
export DYLD_LIBRARY_PATH='/Users/cuddigan/Github/systemshock/build_ext/built_sdl/lib'
./systemshock "$@"
