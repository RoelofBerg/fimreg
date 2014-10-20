@echo off
SET TARGETBOARD=DSPC8681E
SET MIN_DSPID=0
IF %TARGETBOARD%==DSPC8681E (SET MAX_DSPID=3) ELSE (SET MAX_DSPID=7)
SET DSPLOADER=%ADVANTECH_DRIVER_PATH%\dsp_loader.exe
SET MASTEROUT="..\..\dsp\evmc6678l\master\%BUILDCONFIG%\image_processing_evmc6678l_master.out"
SET MASTERENTRY="0x0c000000"
SET SLAVEOUT="..\..\dsp\evmc6678l\slave\%BUILDCONFIG%\image_processing_evmc6678l_slave.out"
SET SLAVEENTRY="0x0c100000"

@echo off
rem load slave core image

for /l %%i in (%MIN_DSPID% 1 %MAX_DSPID%) do (
     for /l %%l in (1, 1, 7) do (
 	    %DSPLOADER% load %%i %%l %SLAVEENTRY% %SLAVEOUT%
 	)
 )

rem load master core image
for /l %%i in (%MIN_DSPID% 1 %MAX_DSPID%) do (%DSPLOADER% load %%i 0 %MASTERENTRY% %MASTEROUT%)
GOTO END

:END


