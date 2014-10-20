----------------------------------------------------------------------------------------

Once you have the drivers installed, the binaries built, you want to see something.
Here are the scripts you need to start an actual image registration.

----------------------------------------------------------------------------------------

First boot the Advantech DSPC-8681 DSPs with the image registration firmware. (Repeat this
after each system boot).

Either use
  BootDSPsWithDebugConsole.bat	   (With target-console debug output, occupies the shell)
or
  BootDSPsWithoutDebugConsole.bat  (No target debug output, returns to the shell)

----------------------------------------------------------------------------------------

Then execute as many image registrations as you like.

For example by
  StartExampleRegistration.bat   (One 4096x4096 test image registration, with image display, occupies the shell)
or
  StartFullMeasurementSeries.bat (Several subsequent image registrations, no image display (just console output), returns to the shell)
or
  Start an IDE debug session     (The projects should be properly configured. Otherwise mimic StartExampleRegistration.bat in the IDE)

----------------------------------------------------------------------------------------
