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

#pragma once

//Common includes for all matlab generated files (used when calling c-functions directly from matlab code)
#include "matlab_timing_utils.h"
#include "matlab_sendToTarget.h"
#include "matlab_jacobianOnTarget.h"
#include "matlab_ssdOnTarget.h"
#include "matlab_startJacobianOnTarget.h"
#include "matlab_startSsdOnTarget.h"
#include "matlab_waitUntilTargetReady.h"
#include "matlab_startNotifyFinishedOnTarget.h"
#include "matlab_c_ssdRigid2D.h"

#include "TimingUtils.h"

//Headers needed by the matlab generated codefiles (esp. when the myprintf is evaluated as inline function)
#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER) 
typedef signed __int8 int8_t; 
typedef signed __int16 int16_t; 
typedef signed __int32 int32_t; 
typedef signed __int64 int64_t; 
typedef unsigned __int8 uint8_t; 
typedef unsigned __int16 uint16_t; 
typedef unsigned __int32 uint32_t; 
typedef unsigned __int64 uint64_t;
#else
//Linux
#include <stdint.h>
#endif 
