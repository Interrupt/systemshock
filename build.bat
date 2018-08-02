@REM Initial build.bat for Appveyor

cmake -G "Unix Makefiles" .
make -j2 systemshock
