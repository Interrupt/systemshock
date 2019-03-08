#!/bin/bash
export DYLD_LIBRARY_PATH="$(pwd)/lib"
./systemshock "$@"
