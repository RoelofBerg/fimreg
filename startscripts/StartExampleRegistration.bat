@echo off
pushd ..\cpu\x64\release
fimreg.exe --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp --dsps 4
popd
