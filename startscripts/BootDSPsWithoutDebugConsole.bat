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

echo \nDSPs ready for executing image registrations\n


