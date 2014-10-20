/* =====================================
=== FIMREG - Fast Image Registration ===
========================================

Written by Roelof Berg with support from Lars Koenig and Jan Ruehaak.
Documentation: http://www.embedded-software-architecture.com/fimreg.html
Contact: rberg@berg-solutions.de

THIS IS A RESEARCH PROTOTYPE WITH LIMITED CODE QUALITY.

------------------------------------------------------------------------------

Copyright (c) 2014, RoelofBerg
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

#include "stdafx.h"

/*
 * ssd.cpp
 *
 * Code generation for function 'ssd'
 *
 * C source code generated on: Fri Jul 06 11:48:06 2012
 *
 */

#ifdef USE_ROELOF_CODE

/* Include files */
#include "calcDSPLayout.h"
#include "jacobian.h"
#include "ssd.h"
#include "fimreg_rtwutil.h"

/* Custom Source Code */
#include "pseudo_stdafx.h"             //precompiled header not possible because of include position of matlab

#include "omp.h"
#include "stdio.h"

/* Type Definitions */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */

/* Function Definitions */

real32_T ssd(const real32_T w[3], const uint32_T BoundBox[4], const uint32_T MarginAddon[3], const real32_T
             DSPRange[4], const emxArray_uint8_T *Tvec, const emxArray_uint8_T
             *Rvec, const uint32_T d)
{
  const real32_T y = (real32_T)d / 2.0F;

  /* Bildbreite incl. Margin */
  const uint32_T width = ((BoundBox[1] - BoundBox[0]) + MarginAddon[0]) + 1U;

    //Apply margin
  const uint32_T uImageStart = width * MarginAddon[1];
  uint8_T* pTData = &Tvec->data[uImageStart];

  //Pointer to reference image
  uint8_T* pRData=Rvec->data;

  //printf("SSD MarginAddon %u %u %u %u\n", MarginAddon[0], MarginAddon[1], MarginAddon[2], MarginAddon[3]);
  //printf("SSD ImageStart=%u width=%u\n", uImageStart, width);
  //printf("SSD TAlloc=%u\n", Tvec->size);

  /* +1 because last entry marks last INCLUDED pixel */
  /* height = uint32(BoundBox(4)-BoundBox(3)+1); */
  /* ======================================================================== */
  /* = Registerweise statt matrizenbasiert */
  /* ======================================================================== */
  const real32_T FP_idx_0 = cosf(w[0]);
  const real32_T FP_idx_1 = -sinf(w[0]);
  const real32_T FP_idx_3 = sinf(w[0]);
  const real32_T FP_idx_4 = cosf(w[0]);
  real32_T SSD = 0.0F;

  /* Folgende beiden For-Loops laufen 1..mn mal durch das pixelmittige Koordinatengitter */
  /* Dabei zeigt i auf die Position von 1..mn */
  const int32_T i2 = (int32_T)(DSPRange[3] + (1.0F - DSPRange[2]));
  const int32_T i3 = (int32_T)(DSPRange[1] + (1.0F - DSPRange[0]));
  int32_T X_mni;

#pragma omp parallel for reduction(+:SSD) shared(FP_idx_0, FP_idx_1, FP_idx_3, FP_idx_4, y, width, i2, i3, w, BoundBox, DSPRange, pTData, pRData, d) private(X_mni)
  for (X_mni = 0; X_mni < i2 ; X_mni++) {
	  real32_T b_X_mni;
  	  int32_T X_i;

    b_X_mni = DSPRange[2] + (real32_T)X_mni;
    for (X_i = 0; X_i < i3; X_i++) {

  	  real32_T FA_mni;
  	  real32_T FA_i;
  	  uint32_T Ax;
  	  uint32_T Ay;
  	  real32_T k11;
  	  real32_T k12;
  	  real32_T k21;
  	  real32_T k22;
  	  uint32_T yOffTop;
  	  uint32_T yOffBottom;

  	  //This was once a continuously incremented variable i=1...
  	  //Because we added OpenMP
  	  int32_T i = (X_mni*i3) + X_i;

   	  FA_mni = DSPRange[0] + (real32_T)X_i;

      /* Die Zeilenvektoren FP(1)*X_i und FP(4)*X_i könnten vorberechnet */
      /* werden (mit FP(3) und FP(6), sowie +s schon mit drin ... */
      FA_i = ((FP_idx_0 * FA_mni + FP_idx_1 * b_X_mni) + w[1]) + (y + 0.5F);
      FA_mni = ((FP_idx_3 * FA_mni + FP_idx_4 * b_X_mni) + w[2]) + (y + 0.5F);

      /* +s weil Drehpunkt in Bildmitte */
      //RBEx Ax = (uint32_T)rt_roundf_snf(floorf(FA_i));
	  Ax = (uint32_T)(FA_i);
      //RBEx Ay = (uint32_T)rt_roundf_snf(floorf(FA_mni));
	  //RBEx 20.3 s -> 18.17 s
	  Ay = (uint32_T)(FA_mni);

    /* ToDo: Folgendes kam mit der Bildaufteilung rein. performance */
    /* prüfen */
    yOffTop = (Ay - BoundBox[2]) * width;
    yOffBottom = ((Ay - BoundBox[2]) + 1U) * width;
    Ay = (Ax - BoundBox[0]) + 1U;
    Ax = Ay + 1U;

    /* todo: Warum ist die teure Operation /255 notwendig ? Kann man */
    /* die vorklammern ? */
    k11 = (real32_T)pTData[(int32_T)(yOffTop + Ay) - 1] / 255.0F;
    k12 = (real32_T)pTData[(int32_T)(yOffTop + Ax) - 1] / 255.0F;
    k21 = (real32_T)pTData[(int32_T)(yOffBottom + Ay) - 1] / 255.0F;
    k22 = (real32_T)pTData[(int32_T)(yOffBottom + Ax) - 1] / 255.0F;

      /* Interpolation ToDo: Cast statt Floor ?*/
      FA_i -= floorf(FA_i);
      FA_mni -= floorf(FA_mni);

      /* residuum */
      /* /1 oder /2 ? */
      real32_T imd = (((k11 * (1.0F - FA_i) * (1.0F - FA_mni) + k12 * FA_i * (1.0F
    	        - FA_mni)) + k21 * (1.0F - FA_i) * FA_mni) + k22 * FA_i * FA_mni) -
    	                  (real32_T)pRData[i] / 255.0F;

	  SSD += imd*imd;
    }
  }
  //printf("SSD=%f\n", SSD);

  return SSD/2.0F;
}

/* End of code generation (ssd.cpp) */
#endif //USE_ROELOF_CODE