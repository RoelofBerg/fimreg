/* =====================================
=== FIMREG - Fast Image Registration ===
========================================

Written by Roelof Berg, Berg Solutions (rberg@berg-solutions.de) with support from
Lars Koenig, Fraunhofer MEVIS (lars.koenig@mevis.fraunhofer.de) Jan Ruehaak, Fraunhofer
MEVIS (jan.ruehaak@mevis.fraunhofer.de).

THIS IS A LIMITED RESEARCH PROTOTYPE. Documentation: www.berg-solutions.de/fimreg.html

------------------------------------------------------------------------------

Copyright (c) 2014, Roelof Berg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the owner nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------------*/

/**
* stdafx.h : include file for standard system include files,
* or project specific include files that are used frequently, but
* are changed infrequently
*/

/**
*  @file stdafx.h
*
*  \brief Common declarations for all .cpp files (precompiled headerfile).
*
*  Common declarations for all .cpp files (e.g. common includes)
*  Can be used as pre-compiled header file. Had to be included AS
*  FIRST FILE in every .cpp file.
*
*  \author Roelof Berg
*/

#pragma once

//Versioning
#include "targetver.h"

//StdLib and Runtime Libs
#include <stdio.h>
#if WIN32
	//Windows specific headers
	#include <iostream>
	#include <windows.h>
#else
	//Linux specific headers
	#include <unistd.h>
	#include <string.h>
	#include <stdlib.h>
	#include <errno.h>				//Note: Not reentrant

	//Some quick hacks to make WinAPI code 'compatible' (...) to linux
    inline void Sleep(int ms) {usleep(ms*1000);}
#endif
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>

//STD (C++ Standard Library, official part of the c++ language standard)
#include <string>
using namespace std;
using std::string;
using std::stringstream;
using std::istream;
using std::ostream;
#include <vector>
#include <map>

//BOOST library (This application makes little usage of boost. If you don't want to use it should be a matter of one hour to get rid of it.)
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/progress.hpp>
#include <boost/algorithm/string/trim.hpp>

//Common includes of our own files (static helper classes)
#include "CLogger.h"

//OpenCV (image processing library, only used for loading and displaying/storing the images. If you don't like it it should be just a matter of one hour to get rid of it - if you have
//an image in- and output alternative readyly available)
#include <cv.h>
#include <highgui.h>

//Commonly used application headers
#include "fimreg_common.h"

//DSP Clock 1.25 GHz (only used for the debug output regarding the calculation duration of partial routines on the target)
const unsigned int DSP_CLOCKRATE = 1250000000;

//Type of the length descriptor in the DMA buffer
typedef uint32_t t_buflen;
