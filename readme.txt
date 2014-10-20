fimreg
=====

Fast Image Registration

Performs a highly efficient Image Registration by using an Advantech DSPC-8681 PCIe expansion card. This is
a supplementary software for our article in the Springer Journal of Realtime Image Processing with the title:
"Highly efficient image registration using a distributed multicore DSP architecture" which is available for
purchase at: TODO ADD LINK, CORRECT TITLE (also in readme.md)

If you want to understand what the application does this paper is the best starting point. Please be aware
that this software is a limited research prototype. If you need industrial quality please contact one of
the authors.

Related projects: https://github.com/RoelofBerg/limereg.git (Lightweight Image Registration. Same as fimreg
but without the demand for special hardware.)

INSTALLATION: See install.txt

OPERATION:
- Start one of the boot-scripts in the folder named 'startscripts'
- Check operation by using one of the example scripts 'start...bat' (compare the output to our paper)
- Execute <fimreg-install-folder>\cpu\x64\Release\fimreg.exe --help for more instructions how to perform your
  own image registrations. The images have to be 2D, square dimension (e.g. 1000x1000 pixels) and grayscale
