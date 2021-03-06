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

/*
 * shrinkImageDSP.cpp
 *
 * CODE GENERATED BY MATLAB CODER (THE HUMAN READABILITY IS THEREFORE LIMITED)
 *
 */

#include "stdafx.h"

/* Include files */
#include "rt_nonfinite.h"
#include "calcDSPLayout.h"
#include "diffimg.h"
#include "dstr_ssd.h"
#include "gaussnewton.h"
#include "gen_example_data.h"
#include "generatePyramidPC.h"
#include "get_current_time_in_sec.h"
#include "jacobian.h"
#include "jacobianOnTarget.h"
#include "myprintf.h"
#include "notiifyFinishedOnTarget.h"
#include "sendToTarget.h"

#include "ssd.h"
#include "ssdOnTarget.h"
#include "start_jacobianOnTarget.h"
#include "start_ssdOnTarget.h"
#include "transform.h"
#include "transmitImageData.h"
#include "waitUntilTargetReady.h"
#include "mpower.h"
#include "shrinkImageDSP.h"
#include "fimreg_emxutil.h"
#include "fimreg_rtwutil.h"

/* Custom Source Code */
#include "pseudo_stdafx.h"             //precompiled header not possible because of include position of matlab

#include "omp.h"					  //OpenMP multiprocessor (if you don't like OpenMP and compile for a single core just remove this header definition and the #pragma omp instructions)

/* Type Definitions */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */

/* Function Definitions */
void shrinkImageDSP(const uint8_T *Img, const uint32_T ImgDim[4], uint32_T SubArea[4],
                   uint8_T *ImgSmall)
{
  /* ToDo: When SubArea is > ImgDim (in some way), only the first for-loop is */
  /* necessary (without the -1 in the end-index) because the margin can be */
  /* kind of re-used ... */

  uint32_T xWidth = ImgDim[1] - ImgDim[0] + 1U;
  uint32_T xWidthSmall = (uint32_T)(real32_T)ceil((real32_T)xWidth / 2.0F);
  uint32_T yHeight = ImgDim[3] - ImgDim[2] + 1;

  int32_T loopStop = int32_T((SubArea[3]-1)>>1)+1;
  const uint32_T yEvenOdd = ((SubArea[2]-1) & 0x1);	//todo: Maybe unnecessary on the dsp when y+=2 loops are equally fast as y++
  for (int32_T y = ((SubArea[2]-1)>>1); y < loopStop; y++) {
    uint32_T iy = (y<<1) + yEvenOdd;
	uint32_T pOut = y * xWidthSmall;
    for (uint32_T x = (SubArea[0]-1); x < (SubArea[1]-1); x += 2U) {
	  uint32_T pIn1 = iy * xWidth + x;
	  uint32_T pIn2 = pIn1 + xWidth;
      uint32_T A = 
				  (uint32_T)Img[pIn1]
				+ (uint32_T)Img[pIn1 + 1]
				+ (uint32_T)Img[pIn2]
				+ (uint32_T)Img[pIn2 + 1];

      ImgSmall[pOut + (x>>1)] = (uint8_T)(A>>2);
    }
  }

  //todo: the for loops below can also be openmp ified
  if (!((SubArea[1]-SubArea[0]) & 0x1)) {	//odd width ?
    for (uint32_T iy = (SubArea[2]-1); iy < (SubArea[3]-1); iy += 2) {
      uint32_T A = (uint32_T)Img[iy * xWidth + SubArea[1] - 1]
	             + (uint32_T)Img[(iy+1) * xWidth + SubArea[1] - 1];
      
      ImgSmall[(iy>>1) * xWidthSmall + (SubArea[1]>>1)]
	    = (uint8_T)(A >> 2);
    }
  }

  if (!((SubArea[3]-SubArea[2]) & 0x1)) {	//odd height?
    uint32_T iy = (yHeight >> 1)-1;
    for (uint32_T x = (SubArea[0]-1); x < (SubArea[1]-1); x += 2U) {
      uint32_T A = (uint32_T)Img[(SubArea[3]-1) * xWidth + x]
                 + (uint32_T)Img[(SubArea[3]-1) * xWidth + x + 1];

      ImgSmall[iy * xWidthSmall + (x>>1)]
		= (uint8_T)(A >> 2);
    }

    if (!((SubArea[1]-SubArea[0]) & 0x1)) {	//odd width (and height)
      uint32_T y = (SubArea[3]-1) * xWidth;

      ImgSmall[iy * xWidthSmall + (SubArea[1] >> 1)]
		= Img[y + SubArea[1] - 1] >> 2;
    }
  }
}

/* End of code generation (shrinkImageDSP.cpp) */
