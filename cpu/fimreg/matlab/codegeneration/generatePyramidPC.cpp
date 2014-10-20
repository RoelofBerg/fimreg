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
 * generatePyramidPC.cpp
 *
 * Code copied from generated code of 'generatePyramid'
 *
 */

/* Include files */

#include "stdafx.h"

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
#include "shrinkImageDSP.h"
#include "fimreg_emxutil.h"
#include "fimreg_rtwutil.h"

/* Custom Source Code */
#include "pseudo_stdafx.h"             //precompiled header not possible because of include position of matlab

/* Type Definitions */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */


#ifdef DUMP_IMAGES				//For debugging: dump the particular pyramid generation steps
typedef uint8_T t_pixel;
typedef uint32_T uint32_t;
#include "raw_image_loader.h"
static uint32_T imgNoT=0;
static uint32_T imgNoR=0;
void dumpImage(const uint8_T* start, const uint8_T* end, const uint32_T xDim, const uint32_T yDim, const uint32_T yStart, const uint32_T yEnd, const char imgType, const uint32_T imgNo)
{
	//Use the matlab script convAll to convert the output debug images to .bmp files ... (copy the images to the folder .../matlab/img beforehand)

	//Assemble filename
	char sNameFormat[] = "_PIC%03d.raw";
	sNameFormat[0] = imgType;
	char sFilename[sizeof(sNameFormat)/*this is enough*/];
	sprintf(sFilename, sNameFormat, imgNo);

	//Set marker to image start
	const uint32_T markerWidth = 20;
	uint8_T topBuf[markerWidth], bottomBuf[markerWidth];
	memcpy(topBuf, &start[yStart*xDim], markerWidth);
	memset((void*)&start[yStart*xDim], 0xff, markerWidth);
	memcpy(bottomBuf, &start[yEnd*xDim], markerWidth);
	memset((void*)&start[yEnd*xDim], 0xff, markerWidth);

	//check
	if(end-start != xDim*yDim)
	{
		printf("INVALID IMAGE DIMENSIONS (len=%d, xDim=%d, yDim=%d).\n", end-start, xDim, yDim);
	}

	//Save to file
	SaveRawImage(sFilename, (t_pixel*)start, end-start, xDim, yDim);

	//Remove markers
	memcpy((void*)&start[yStart*xDim], topBuf, markerWidth);
	memcpy((void*)&start[yEnd*xDim], bottomBuf, markerWidth);
}
#endif

inline uint32_T GetBoundBoxWidth(emxArray_uint32_T *BoundBox, uint32_T Level)
{
	return  BoundBox->data[(int32_T)(Level-1) + BoundBox->size[0] /* *1 */]
			-
			BoundBox->data[(int32_T)(Level-1) /* + BoundBox->size[0] *0 */]
			+ 1
			;
}

inline uint32_T GetBoundBoxHeight(emxArray_uint32_T *BoundBox, uint32_T Level)
{
	return  BoundBox->data[(int32_T)(Level-1) + BoundBox->size[0] *3]
			-
			BoundBox->data[(int32_T)(Level-1) + (BoundBox->size[0] << 1) /* *2 */]
			+ 1
			;
}

/* Function Definitions */
void generatePyramidPC(const emxArray_uint8_T *Tvec, emxArray_uint32_T *BoundBox, emxArray_uint32_T *MarginAddition, const emxArray_uint8_T *Rvec,
						emxArray_real32_T *DSPRange, uint32_T LevelCount, emxArray_uint32_T *TLvlPtrs, uint32_T TSizeWithPyramid, emxArray_uint32_T *RLvlPtrs,
						uint32_T RSizeWithPyramid, uint8_T* Tvec_Wo_Margins
						)
{
  uint32_T RSizeWoPyramid;
  int32_T i39;
  int32_T loop_ub;
  uint32_T BoxWMargin[4];
  uint32_T RLvlPtrAkt;
  uint32_T TWidth;
  uint32_T THeight;
  uint32_T TLvlPtrAkt;
  uint32_T Level;
  real32_T A[4];
  real32_T x[2];
  real32_T ImgDim[2];
  uint32_T RelevantArea[4];
  int32_T i24;
  uint32_T THeightBig;
  uint32_T TWidthBig;
  int32_T i41;
  int32_T i40;

  /* Allocate memory (1 vector for each R and T containing the image data */
  /* of all levels. */

  /* +1 because the target will floor (matlab ceils) */
  i39 = RLvlPtrs->size[0] * RLvlPtrs->size[1];
  RLvlPtrs->size[0] = (int32_T)LevelCount;
  RLvlPtrs->size[1] = 2;
  emxEnsureCapacity((emxArray__common *)RLvlPtrs, i39, (int32_T)sizeof(uint32_T));
  loop_ub = ((int32_T)LevelCount << 1) - 1;
  for (i39 = 0; i39 <= loop_ub; i39++) {
    RLvlPtrs->data[i39] = 0U;
  }

  RLvlPtrAkt = 1U;
  TWidth = GetBoundBoxWidth(BoundBox, 1) + MarginAddition->data[0];

  /* Width includes only one horizontal margin (which is shared for left and right) */
  THeight = (GetBoundBoxHeight(BoundBox, 1) + MarginAddition->
             data[MarginAddition->size[0]]) + MarginAddition->
    data[MarginAddition->size[0] << 1];

  /* +1 because the target will floor (matlab ceils) */
  i39 = TLvlPtrs->size[0] * TLvlPtrs->size[1];
  TLvlPtrs->size[0] = (int32_T)LevelCount;
  TLvlPtrs->size[1] = 2;
  emxEnsureCapacity((emxArray__common *)TLvlPtrs, i39, (int32_T)sizeof(uint32_T));
  loop_ub = ((int32_T)LevelCount << 1) - 1;
  for (i39 = 0; i39 <= loop_ub; i39++) {
    TLvlPtrs->data[i39] = 0U;
  }

  TLvlPtrAkt = 1U;

  Level = 1U;
  while (Level <= LevelCount) {
    /* RDimShrinked = ceil((RDim+0.5)/2)-0.5 */
    if (Level > 1U) {
      for (i39 = 0; i39 < 4; i39++) {
        A[i39] = DSPRange->data[((int32_T)Level + DSPRange->size[0] * i39) - 2]
          + 0.5F;
      }

      for (i39 = 0; i39 < 4; i39++) {
        A[i39] = (real32_T)ceil(A[i39] / 2.0F);
      }

      for (i39 = 0; i39 < 4; i39++) {
        DSPRange->data[((int32_T)Level + DSPRange->size[0] * i39) - 1] = A[i39]
          - 0.5F;
      }
    }

    /* RShrinked = shrinkImage(R, RDim) */
    for (i39 = 0; i39 < 2; i39++) {
      x[i39] = DSPRange->data[((int32_T)Level + DSPRange->size[0] * (1 + (i39 <<
        1))) - 1] - DSPRange->data[((int32_T)Level + DSPRange->size[0] * (i39 <<
        1)) - 1];
    }

    for (loop_ub = 0; loop_ub < 2; loop_ub++) {
      ImgDim[loop_ub] = (real32_T)fabs(x[loop_ub]) + 1.0F;
    }

    RSizeWoPyramid = (uint32_T)rt_roundf_snf(ImgDim[0] * ImgDim[1]);
    RLvlPtrs->data[(int32_T)Level - 1] = RLvlPtrAkt;
    RLvlPtrs->data[((int32_T)Level + RLvlPtrs->size[0]) - 1] = (RLvlPtrAkt +
      RSizeWoPyramid) + MAX_uint32_T;
    if ((int32_T)Level > 1) {
      RelevantArea[0] = 1U;
      RelevantArea[1] = (uint32_T)rt_roundf_snf((DSPRange->data[((int32_T)Level
        + DSPRange->size[0]) - 2] - DSPRange->data[(int32_T)Level - 2]) + 1.0F);
      RelevantArea[2] = 1U;
      RelevantArea[3] = (uint32_T)rt_roundf_snf((DSPRange->data[((int32_T)Level
        + DSPRange->size[0] * 3) - 2] - DSPRange->data[((int32_T)Level +
        (DSPRange->size[0] << 1)) - 2]) + 1.0F);

      if (RLvlPtrs->data[(int32_T)Level - 2] > RLvlPtrs->data[((int32_T)Level +
           RLvlPtrs->size[0]) - 2]) {
        i39 = 0;
      } else {
        i39 = (int32_T)RLvlPtrs->data[(int32_T)Level - 2] - 1;
      }

      if (RLvlPtrs->data[(int32_T)Level - 1] > RLvlPtrs->data[((int32_T)Level +
           RLvlPtrs->size[0]) - 1]) {
        i41 = 0;
      } else {
        i41 = (int32_T)RLvlPtrs->data[(int32_T)Level - 1] - 1;
      }

      shrinkImageDSP(&Rvec->data[i39], RelevantArea, RelevantArea, &Rvec->data[i41]);
	  #ifdef DUMP_IMAGES
	  dumpImage(&Rvec->data[i39], &Rvec->data[i41], RelevantArea[1]-RelevantArea[0]+1, RelevantArea[3]-RelevantArea[2]+1, RelevantArea[2]-1, RelevantArea[3]-1, 'R', imgNoR++);
	  #endif
	}

    RLvlPtrAkt += RSizeWoPyramid;

    /* TDimShrinked = TDim / 2 */
    if ((int32_T)Level > 1) {
      /* Sum divide and sub to behave exactly like shrinkImage (it is */
      /* not ok to ceil/2 MardinAddition and BoundBox indipendently */
      /* here because of the ceil operation ... */
      TWidthBig = GetBoundBoxWidth(BoundBox, Level-1) + MarginAddition->data[(int32_T)Level - 2];

      /* Width includes only one horizontal margin (which is shared for left and right) */
      THeightBig = (GetBoundBoxHeight(BoundBox, Level-1) + MarginAddition->data[((int32_T)Level +
        MarginAddition->size[0]) - 2]) + MarginAddition->data[((int32_T)Level +
        (MarginAddition->size[0] << 1)) - 2];
      BoxWMargin[0] = 1U;
      BoxWMargin[1] = TWidthBig;
      BoxWMargin[2] = 1U;
      BoxWMargin[3] = THeightBig;

      /* Formulas taken from shrinkImage.m (maybe we can pass just this */
      /* results to shrinkImage ... so we don't calculate duplicates) */
      TWidth = (uint32_T)(real32_T)ceil((real32_T)BoxWMargin[1] / 2.0F);
      THeight = (uint32_T)(real32_T)ceil((real32_T)BoxWMargin[3] / 2.0F);
      for (i24 = 0; i24 < 4; i24++) {
        A[i24] = (real32_T)BoundBox->data[((int32_T)Level + BoundBox->size[0] *
          i24) - 2];
      }

      for (i24 = 0; i24 < 4; i24++) {
        A[i24] = (real32_T)ceil(A[i24] / 2.0F);
      }

      for (i24 = 0; i24 < 4; i24++) {
        BoundBox->data[((int32_T)Level + BoundBox->size[0] * i24) - 1] =
          (uint32_T)rt_roundf_snf(A[i24]);
      }

      MarginAddition->data[(int32_T)Level - 1] = ((TWidth - BoundBox->data
        [((int32_T)Level + BoundBox->size[0]) - 1]) + BoundBox->data[(int32_T)
        Level - 1]) - 1U;
      RSizeWoPyramid = ((THeight - BoundBox->data[((int32_T)Level +
        BoundBox->size[0] * 3) - 1]) + BoundBox->data[((int32_T)Level +
        (BoundBox->size[0] << 1)) - 1]) - 1U;
      THeightBig = RSizeWoPyramid >> 1;
      if (RSizeWoPyramid - (THeightBig << 1) > 0U) {
        THeightBig++;
      }

	  MarginAddition->data[((int32_T)Level + MarginAddition->size[0]) - 1] = 
		  MarginAddition->data[((int32_T)(Level-1) + MarginAddition->size[0]) - 1] / 2;

      MarginAddition->data[((int32_T)Level + (MarginAddition->size[0] << 1)) - 1]
        = (((THeight - BoundBox->data[((int32_T)Level + BoundBox->size[0]) - 1])
            + BoundBox->data[(int32_T)Level - 1]) - MarginAddition->data
           [((int32_T)Level + MarginAddition->size[0]) - 1]) - 1U;

	  //debugging:
      #ifdef _TRACE_OUTPUT_
  	  for(uint32_T mlo_l=0; mlo_l<LevelCount; mlo_l++) //level
	  	for(uint32_T mlo_i=0; mlo_i<3; mlo_i++) //0=r 1=u 2=d
	  		printf("Margin[%d, %d] = %d\n", mlo_l, mlo_i, MarginAddition->data[((int32_T)mlo_l + (MarginAddition->size[0] * mlo_i))]);
	  #endif
    }

    /* TShrinked = shrinkImage(T, TDim) */
    RSizeWoPyramid = TWidth * THeight;
    TLvlPtrs->data[(int32_T)Level - 1] = TLvlPtrAkt;
    TLvlPtrs->data[((int32_T)Level + TLvlPtrs->size[0]) - 1] = (TLvlPtrAkt +
      RSizeWoPyramid) - 1U;
    if ((int32_T)Level > 1) {
      /* Shrink image */

	  //Relevant area
      for (i39 = 0; i39 < 4; i39++) {
        RelevantArea[i39] = BoundBox->data[((int32_T)Level + BoundBox->size[0] * i39) - 2];
      }
      THeightBig = MarginAddition->data[((int32_T)Level + MarginAddition->size[0]) - 2];
      for (i39 = 0; i39 < 2; i39++) {
        RelevantArea[2 + i39] = BoundBox->data[((int32_T)Level + BoundBox->size[0] * (2 + i39)) - 2] + THeightBig;
      }

      if (TLvlPtrs->data[(int32_T)Level - 2] > TLvlPtrs->data[((int32_T)Level +
           TLvlPtrs->size[0]) - 2]) {
        i39 = 0;
      } else {
        i39 = (int32_T)TLvlPtrs->data[(int32_T)Level - 2] - 1;
      }

      if (TLvlPtrs->data[(int32_T)Level - 1] > TLvlPtrs->data[((int32_T)Level +
           TLvlPtrs->size[0]) - 1]) {
        i41 = 0;
      } else {
        i41 = (int32_T)TLvlPtrs->data[(int32_T)Level - 1] - 1;
      }

	  shrinkImageDSP(&Tvec->data[i39], BoxWMargin, RelevantArea, &Tvec->data[i41]);
	  #ifdef DUMP_IMAGES
	  dumpImage(&Tvec->data[i39], &Tvec->data[i41], BoxWMargin[1]-BoxWMargin[0]+1, BoxWMargin[3]-BoxWMargin[2]+1, RelevantArea[2]-1, RelevantArea[3]-1, 'T', imgNoT++);
	  #endif

      TLvlPtrAkt += RSizeWoPyramid;
    } else {

      /* Add margins */
      RSizeWoPyramid = 1U;
      TLvlPtrAkt += MarginAddition->data[MarginAddition->size[0]] * TWidth;

      /* Add top margin */
      for (uint32_T iRow = 1U; iRow <= BoundBox->data[BoundBox->size[0] * 3]; iRow++) {
        uint32_T u0 = (RSizeWoPyramid + BoundBox->data[BoundBox->size[0]]) - 1U;
        if (RSizeWoPyramid > u0) {
          i39 = 1;
          i40 = 0;
        } else {
          i39 = (int32_T)RSizeWoPyramid;
          i40 = (int32_T)u0;
        }

        u0 = (TLvlPtrAkt + BoundBox->data[BoundBox->size[0]]) - 1U;
        if (TLvlPtrAkt > u0) {
          i41 = 0;
        } else {
          i41 = (int32_T)TLvlPtrAkt - 1;
        }

        loop_ub = i40 - i39;
		memcpy(&Tvec->data[i41], &Tvec_Wo_Margins[i39 - 1], loop_ub+1);

        RSizeWoPyramid += BoundBox->data[BoundBox->size[0]];
        TLvlPtrAkt = (TLvlPtrAkt + BoundBox->data[BoundBox->size[0]]) +
          MarginAddition->data[0];

        /* Add horizontal margin */
      }

      TLvlPtrAkt += MarginAddition->data[MarginAddition->size[0] << 1] * TWidth;

      /* Add bottom margin */
    }

    Level++;
  }

}


/* End of code generation (generatePyramidPC.cpp) */
