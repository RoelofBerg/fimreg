@echo off
SET FIMREG=fimreg.exe
pushd ..\cpu\x64\release

%FIMREG% --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp  --nogui --dsps 4
timeout 2
%FIMREG% --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp  --nogui --dsps 4
timeout 2
%FIMREG% --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp  --nogui --dsps 4
timeout 2

%FIMREG% --tfile ..\..\testimg\T_2048.bmp --rfile ..\..\testimg\R_2048.bmp --nogui --dsps 4
timeout 2
%FIMREG% --tfile ..\..\testimg\T_2048.bmp --rfile ..\..\testimg\R_2048.bmp --nogui --dsps 4
timeout 2
%FIMREG% --tfile ..\..\testimg\T_2048.bmp --rfile ..\..\testimg\R_2048.bmp --nogui --dsps 4
timeout 2

%FIMREG% --tfile ..\..\testimg\T_1024.bmp --rfile ..\..\testimg\R_1024.bmp --nogui --dsps 4
timeout 2
%FIMREG% --tfile ..\..\testimg\T_1024.bmp --rfile ..\..\testimg\R_1024.bmp --nogui --dsps 4
timeout 2
%FIMREG% --tfile ..\..\testimg\T_1024.bmp --rfile ..\..\testimg\R_1024.bmp --nogui --dsps 4
timeout 2

%FIMREG% --tfile ..\..\testimg\T_512.bmp --rfile ..\..\testimg\R_512.bmp --nogui --dsps 4
timeout 2
%FIMREG% --tfile ..\..\testimg\T_512.bmp --rfile ..\..\testimg\R_512.bmp --nogui --dsps 4
timeout 2
%FIMREG% --tfile ..\..\testimg\T_512.bmp --rfile ..\..\testimg\R_512.bmp --nogui --dsps 4
timeout 2

%FIMREG% --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp  --nogui --dsps 1
timeout 2
%FIMREG% --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp  --nogui --dsps 1
timeout 2
%FIMREG% --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp  --nogui --dsps 1
timeout 2

%FIMREG% --tfile ..\..\testimg\T_2048.bmp --rfile ..\..\testimg\R_2048.bmp --nogui --dsps 1
timeout 2
%FIMREG% --tfile ..\..\testimg\T_2048.bmp --rfile ..\..\testimg\R_2048.bmp --nogui --dsps 1
timeout 2
%FIMREG% --tfile ..\..\testimg\T_2048.bmp --rfile ..\..\testimg\R_2048.bmp --nogui --dsps 1
timeout 2

%FIMREG% --tfile ..\..\testimg\T_1024.bmp --rfile ..\..\testimg\R_1024.bmp --nogui --dsps 1
timeout 2
%FIMREG% --tfile ..\..\testimg\T_1024.bmp --rfile ..\..\testimg\R_1024.bmp --nogui --dsps 1
timeout 2
%FIMREG% --tfile ..\..\testimg\T_1024.bmp --rfile ..\..\testimg\R_1024.bmp --nogui --dsps 1
timeout 2

%FIMREG% --tfile ..\..\testimg\T_512.bmp --rfile ..\..\testimg\R_512.bmp --nogui --dsps 1
timeout 2
%FIMREG% --tfile ..\..\testimg\T_512.bmp --rfile ..\..\testimg\R_512.bmp --nogui --dsps 1
timeout 2
%FIMREG% --tfile ..\..\testimg\T_512.bmp --rfile ..\..\testimg\R_512.bmp --nogui --dsps 1
timeout 2

%FIMREG% --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp  --nogui --dsps 0
timeout 2
%FIMREG% --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp  --nogui --dsps 0
timeout 2
%FIMREG% --tfile ..\..\testimg\T_4096.bmp --rfile ..\..\testimg\R_4096.bmp  --nogui --dsps 0
timeout 2

%FIMREG% --tfile ..\..\testimg\T_2048.bmp --rfile ..\..\testimg\R_2048.bmp --nogui --dsps 0
timeout 2
%FIMREG% --tfile ..\..\testimg\T_2048.bmp --rfile ..\..\testimg\R_2048.bmp --nogui --dsps 0
timeout 2
%FIMREG% --tfile ..\..\testimg\T_2048.bmp --rfile ..\..\testimg\R_2048.bmp --nogui --dsps 0
timeout 2

%FIMREG% --tfile ..\..\testimg\T_1024.bmp --rfile ..\..\testimg\R_1024.bmp --nogui --dsps 0
timeout 2
%FIMREG% --tfile ..\..\testimg\T_1024.bmp --rfile ..\..\testimg\R_1024.bmp --nogui --dsps 0
timeout 2
%FIMREG% --tfile ..\..\testimg\T_1024.bmp --rfile ..\..\testimg\R_1024.bmp --nogui --dsps 0
timeout 2

%FIMREG% --tfile ..\..\testimg\T_512.bmp --rfile ..\..\testimg\R_512.bmp --nogui --dsps 0
timeout 2
%FIMREG% --tfile ..\..\testimg\T_512.bmp --rfile ..\..\testimg\R_512.bmp --nogui --dsps 0
timeout 2
%FIMREG% --tfile ..\..\testimg\T_512.bmp --rfile ..\..\testimg\R_512.bmp --nogui --dsps 0
timeout 2

popd