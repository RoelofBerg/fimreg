@echo off
rem path to the advantech driver
SET ADVANTECH_LIGHTNING_PATH="..\Lightning_PCIE"

rem resolve to an absolute path
pushd %ADVANTECH_LIGHTNING_PATH%
set ADVANTECH_LIGHTNING_PATH=%CD%
popd

rem Boot DSPs
pushd bootdsp
call bootdsp.bat
popd

rem Wait until the DSPs have executed memset(.far:PICTUREBUFF, 0) which takes somewhat time when big buffer sizes are used
timeout 2

echo \nDSPs ready for executing image registrations\n

rem open console window
pushd %ADVANTECH_LIGHTNING_PATH%\examples\ipc\pc\x64
dsp_demo.exe console 0 0
popd
