#!/bin/bash
export LD_LIBRARY_PATH="$(pwd)/lib"
./systemshock "$@"
