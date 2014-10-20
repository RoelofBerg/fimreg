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

#include "matlab_c_ssdRigid2D.h"

// TODO: REDUCE HEADERS TO MINIMAL SUBSET
#include <stdio.h>
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/Types.h>
#include <xdc/runtime/Timestamp.h>
#include <ti/sysbios/hal/Cache.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Log.h>
// TODO: REDUCE HEADERS TO MINIMAL SUBSET

#include "logout.h"
#include <stdlib.h>

#ifdef _TRACE_PAPER_FIGURES_
//todo: read from OS config global
const unsigned int DSP_CLOCKRATE_US = 1250000000 / 1000000;	//divide clocks by this value to get time in us
#endif

#ifdef _DO_ERROR_CHECKS_

int gMinAllowedIdx = 0;
int gMaxAllowedIdx = 0;

void CalcMinMaxIndex(const unsigned int MarginAddon[3], const int mT[2], unsigned int uImageStart)
{
	const unsigned int iRightMargin = MarginAddon[0];
	const unsigned int iTopMargin = MarginAddon[1];
	const unsigned int iBottomMargin = MarginAddon[2];
	const unsigned int iImageWidth = mT[0];
	const unsigned int iImageHeight = mT[1];

	const int totalBytes = (int)(//total bytes including zeroed bounding box
		+ iTopMargin*iImageWidth
		+ iImageHeight*iImageWidth
		+ iBottomMargin*iImageWidth
		+ iRightMargin*iTopMargin
		+ iRightMargin*iImageHeight
		+ iRightMargin*iBottomMargin
		);

	gMinAllowedIdx = -((int)uImageStart);
	gMaxAllowedIdx = totalBytes-uImageStart-1;		//Laut Log des CacheInval wäre -1 ggf. unnötig ? Oder wird 1 byte zu viel CacheInvaled ?)
}

#endif

// -----------------------------------------------------------------------------

// #############################################################################
// Function definitions
// #############################################################################

// Mark: -
// Mark: Function value computations

#ifdef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
inline bool isValidIndex(int M1, int M2, int i, int j);
#endif

t_real matlab_c_ssdRigid2D(t_pixel* dataR, int mR[2], t_real omegaR[4],
                            t_pixel* dataT, int mT[2], t_real omegaT[4],
                            const t_real w[3], const unsigned int MarginAddon[3],
							const t_real DSPRange[4], const unsigned int i_from, const unsigned int i_to){

	  #ifdef _TRACE_PAPER_FIGURES_
	  uint32_t time_s = Timestamp_get32();
	  #endif

	  //Apply margin, set pointer fo first pixe containing data
	  /* Bildbreite incl. Margin */
	  //const unsigned int width = ((BoundBox[1] - BoundBox[0]) + MarginAddon[0]) + 1U;
	  const unsigned int width = (mT[0] + MarginAddon[0]);
	  const unsigned int uImageStart = width * MarginAddon[1];
	  dataT = &dataT[uImageStart];

		#ifdef _DO_ERROR_CHECKS_
		  CalcMinMaxIndex(MarginAddon, mT, uImageStart);
		#endif

	  //precompute some values
	  const t_real s = sin(w[0]);
	  const t_real c = cos(w[0]);
	  const t_real precomputedParamA = (w[1] - omegaT[0] + c*omegaR[0] - s*omegaR[2]) + 0.5f*(c - s - 1.0f);
	  const t_real precomputedParamB = (w[2] - omegaT[2] + s*omegaR[0] + c*omegaR[2]) + 0.5f*(s + c - 1.0f);
	  t_real fval = 0.0f;


	  // Variables are declared inside loop on purpose. No speedup was observed when setting up
	  // variables outside and declaring them as private, yet errors can easily remain
	  // undetected that way.

	//#pragma omp parallel for reduction(+: fval) if(useOpenMP)
	  for (int j=i_from; j<i_to/*mR[1]*/; j++) {
	    const int yIndex = j*mR[0];
		//#pragma MUST_ITERATE(5)
		//#pragma MUST_ITERATE(2, ,2)    //ToDo: Would this make the code faster ? As we have padding borders (which might have to be sized +1 pixel below then) it might work to round i to the next multiple of 2  ...
	    for (int i=0; i<mR[0]; i++) {

	      // Compute corresponding template image coordinates belonging to current pixel
	      // shifts for cell-centered discretization are incorporated into precomputedParams
	      const t_real x_voxel = c*i - s*j + precomputedParamA;
	      const t_real y_voxel = s*i + c*j + precomputedParamB;

	      // Subtract current values for R and T

		  //rbe todo: substraction is vice versa here and in the other place. Dies this make sence ?
		  const t_real val =  dataR[yIndex+i] - linearInterPoint2D(dataT, x_voxel, y_voxel, width
			#ifdef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
			, mT[0], mT[1]
			#endif
		  );

	      fval += val*val;
	    }
	  }

	  t_real fRetVal = 0.5f*fval;

	  #ifdef _TRACE_PAPER_FIGURES_
	  uint32_t time_e = Timestamp_get32();
	  uint32_t TimeForDoubleLoop = time_e-time_s;
	  logout("SSD took %d us.\n", TimeForDoubleLoop / DSP_CLOCKRATE_US);
	  #endif

	  return fRetVal;


} // end of computeFunctionValueAffine

// -----------------------------------------------------------------------------

// Mark: -
// Mark: Derivative computations

t_real matlab_c_ssdRigid2D(t_pixel* dataR, int mR[2], t_real omegaR[4],
                                          t_pixel* dataT, int mT[2], t_real omegaT[4],
                                          const t_real w[3],
										  const unsigned int MarginAddon[3],
										  const t_real DSPRange[4],
										  const unsigned int i_from, const unsigned int i_to,
										  t_real grad[3], t_real H[9]){

  	  #ifdef _TRACE_PAPER_FIGURES_
	  uint32_t time_s = Timestamp_get32();
	  #endif

	  //Apply margin, set pointer fo first pixe containing data
	  /* Bildbreite incl. Margin */
	  //const uint32_T width = ((BoundBox[1] - BoundBox[0]) + MarginAddon[0]) + 1U;
	  const unsigned int width = (mT[0] + MarginAddon[0]);
	  const unsigned int uImageStart = width * MarginAddon[1];
	  dataT = &dataT[uImageStart];

		#ifdef _DO_ERROR_CHECKS_
		  CalcMinMaxIndex(MarginAddon, mT, uImageStart);
		#endif

  	  //precompute some values
	  const t_real s = sin(w[0]);
	  const t_real c = cos(w[0]);
	  const t_real precomputedParamA = (w[1] - omegaT[0] + c*omegaR[0] - s*omegaR[2]) + 0.5f*(c - s - 1.0f);
	  const t_real precomputedParamB = (w[2] - omegaT[2] + s*omegaR[0] + c*omegaR[2]) + 0.5f*(s + c - 1.0f);

	  t_real fval = 0.0f;

	  // Declare gradient entries as scalars due to OpenMP array reduction limitations
	  t_real grad_r0 = 0.0f;  t_real grad_r1 = 0.0f;  t_real grad_r2 = 0.0f;

	  t_real HR_00 = 0.0f;  t_real HR_01 = 0.0f;  t_real HR_02 = 0.0f;
	  t_real HR_11 = 0.0f;  t_real HR_12 = 0.0f;
	  t_real HR_22 = 0.0f;

	  // Variables are declared inside loop on purpose. No speedup was observed when setting up
	  // variables outside and declaring them as private, yet errors can easily remain
	  // undetected that way.

	//#pragma omp parallel for reduction(+: fval, grad_r0, grad_r1, grad_r2, HR_00, HR_01, HR_02, HR_11, HR_12, HR_22) if(useOpenMP)
	  for (int j=i_from; j<i_to/*mR[1]*/; j++) {
	    const t_real y_grid = omegaR[2] + (j+0.5f);
	    const int yIndex = j*mR[0];
		//#pragma MUST_ITERATE(3)
		//#pragma MUST_ITERATE(2, ,2)    //ToDo: Would this make the code faster ? As we have padding borders (which might have to be sized +1 pixel below then) it might work to round i to the next multiple of 2  ...
	    for (int i=0; i<mR[0]; i++) {

	      t_real dTx, dTy;

	      // Compute corresponding template image coordinates belonging to current pixel
	      // shifts for cell-centered discretization are incorporated into precomputedParams
	      const t_real x_voxel = c*i - s*j + precomputedParamA;	//ToDo: What is faster ? To loop with a float value and omit the multiplications here ? Or to loop with integer value and to multiply here ?
	      const t_real y_voxel = s*i + c*j + precomputedParamB;

	      // Subtract current values for R and T (factor -1 is included here)
		  const t_real val =  linearInterPoint2D(dataT, x_voxel, y_voxel, dTx, dTy, width
				  #ifdef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
				  , mT[0], mT[1]
				  #endif
				  ) - (dataR[yIndex+i]);

	      // Compute current grid position in world coordinates
	      const t_real x_grid = omegaR[0] + (i+0.5f);

	      const t_real dxx = dTx*x_grid;
	      const t_real dxy = dTx*y_grid;
	      const t_real dx  = dTx;

	      const t_real dyx = dTy*x_grid;
	      const t_real dyy = dTy*y_grid;
	      const t_real dy  = dTy;

	      const t_real term = c*dyx - s*dxx - c*dxy - s*dyy;

	      grad_r0 += val*term;
	      grad_r1 += val*dx;
	      grad_r2 += val*dy;

	      HR_00 += term*term;
	      HR_01 += term*dx;
	      HR_02 += term*dy;

	      HR_11 += dx*dx;
	      HR_12 += dx*dy;

	      HR_22 += dy*dy;

	      fval += val*val;
	    }
	  }

	  // multiply grad with h_bar
	  grad[0] = grad_r0; grad[1] = grad_r1; grad[2] = grad_r2;

	  H[0] = HR_00;  H[1] = HR_01;  H[2] = HR_02;
	  H[1*3+1] = HR_11;  H[1*3+2] = HR_12;  H[2*3+2] = HR_22;

	  // fill lower left part of matrix
	  for (int i=1; i<3; i++){
	    for (int j=0; j<i; j++){
	      H[i*3+j] = H[j*3+i];
	    }
	  }

	  t_real fRetVal = 0.5f*fval;

	  #ifdef _TRACE_PAPER_FIGURES_
	  uint32_t time_e = Timestamp_get32();
	  uint32_t TimeForDoubleLoop = time_e-time_s;
	  logout("SSDJacHess took %d us.\n", TimeForDoubleLoop / DSP_CLOCKRATE_US);
	  #endif

	  return fRetVal;
} // end of computeFunctionValueAndDerivatives


// -----------------------------------------------------------------------------

// Mark: -
// Mark: Routines for linear interpolation

inline t_real linearInterPoint2D(const t_pixel* T, const t_real x_voxel, const t_real y_voxel, const int widthInclMargin
#ifdef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
		,const int ImgWidth, const int ImgHeight
#endif
){
  
  // faster; implements: int   x_int = static_cast<int>(::floor(x_voxel)); the +2/-2 trick ensures correct round-off in the relevant range
  const int   x_int = static_cast<int> ((x_voxel));  //+2) - 2;
  const t_real dx = (x_voxel) - static_cast<t_real>(x_int);
  
  // faster; implements: int   y_int = static_cast<int>(::floor(y_voxel));
  const int   y_int = static_cast<int> ((y_voxel));  //+2) - 2;
  const t_real dy = (y_voxel) - static_cast<t_real>(y_int);
  
  // calculate image values of neighboring points
  const int yIdx=  widthInclMargin*y_int;
#ifndef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
  t_real w00 = static_cast<t_real>(T[x_int   + yIdx   ]);
  t_real w01 = static_cast<t_real>(T[x_int   + yIdx + widthInclMargin]);
  t_real w10 = static_cast<t_real>(T[x_int+1 + yIdx   ]);
  t_real w11 = static_cast<t_real>(T[x_int+1 + yIdx + widthInclMargin]);
#else
  // not initialized on purpose (speed)
  t_real w00;
  t_real w01;
  t_real w10;
  t_real w11;
  if (isValidIndex(ImgWidth-1,ImgHeight-1,x_int,y_int) && isValidIndex(ImgWidth,ImgHeight,x_int+1,y_int+1)){

      w00 = static_cast<t_real>(T[x_int   + yIdx   ]);
      w01 = static_cast<t_real>(T[x_int   + yIdx + widthInclMargin ]);
      w10 = static_cast<t_real>(T[x_int+1 + yIdx   ]);
      w11 = static_cast<t_real>(T[x_int+1 + yIdx + widthInclMargin]);
    }
    else{
      w00 = isValidIndex(ImgWidth, ImgHeight, x_int, y_int)
      ? static_cast<t_real>(T[x_int + yIdx])
      : 0;

      w01 = isValidIndex(ImgWidth, ImgHeight, x_int, y_int+1)
      ? static_cast<t_real>(T[x_int + yIdx + widthInclMargin])
      : 0;

      w10 = isValidIndex(ImgWidth, ImgHeight, x_int+1, y_int)
      ? static_cast<t_real>(T[(x_int+1)+yIdx])
      : 0;

      w11 = isValidIndex(ImgWidth, ImgHeight, x_int+1, y_int+1)
      ? static_cast<t_real>(T[(x_int+1)+yIdx + widthInclMargin])
      : 0;

    }
#endif


#ifdef _DO_ERROR_CHECKS_
	const int minIdx = x_int + yIdx;
	const int maxIdx = x_int+1 + yIdx + widthInclMargin;
	if(minIdx<gMinAllowedIdx || maxIdx>gMaxAllowedIdx)
	{
		logout("ERROR: Accessing invalid pixels in template image.\n");
		exit(0);
	}

	/*
	static int bBegin=0;
	if(0==bBegin)
	{
		logout("### MIN:%d   MAX:%d (len: 0x%x) ###\n", gMinAllowedIdx, gMaxAllowedIdx, gMaxAllowedIdx-gMinAllowedIdx);
		logout("### MINPTR: 0x%x   MAXPTR: 0x%x ###\n", ((int)(&T[0])) + gMinAllowedIdx, ((int)(&T[0])) + gMaxAllowedIdx);
		bBegin=1;
	}
	*/
#endif

  // calculate remainder
  const t_real dx_1 = 1.0f-dx;
  const t_real dy_1 = 1.0f-dy;
  
  return (dx_1*(dy_1*w00+dy*w01) + dx*(dy_1*w10+dy*w11)) / 255.0f;
}
// "Horner scheme"-style, implements
//  result =
//  (1.0-dx)*(1.0-dy)*w00 +
//  (1.0-dx)*(    dy)*w01 +
//  (    dx)*(1.0-dy)*w10 +
//  (    dx)*(    dy)*w11 +

// -----------------------------------------------------------------------------

inline t_real linearInterPoint2D(const t_pixel* T, const t_real x_voxel, const t_real y_voxel, t_real& dTx, t_real& dTy, const int widthInclMargin
#ifdef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
		,const int ImgWidth, const int ImgHeight
#endif
){
  
  // faster; implements: int   x_int = static_cast<int>(::floor(x_voxel)); the +2/-2 trick ensures correct round-off in the relevant range
  const int   x_int = static_cast<int> ((x_voxel)); //+2) - 2;
  const t_real dx = (x_voxel) - static_cast<t_real>(x_int);
  
  // faster; implements: int   y_int = static_cast<int>(::floor(y_voxel));
  const int   y_int = static_cast<int> ((y_voxel));  //+2) - 2;
  const t_real dy = (y_voxel) - static_cast<t_real>(y_int);
  
  // calculate image values of neighboring points
  const int yIdx=  widthInclMargin*y_int;
#ifndef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
  t_real w00 = static_cast<t_real>(T[x_int   + yIdx   ]);
  t_real w01 = static_cast<t_real>(T[x_int   + yIdx + widthInclMargin]);
  t_real w10 = static_cast<t_real>(T[x_int+1 + yIdx   ]);
  t_real w11 = static_cast<t_real>(T[x_int+1 + yIdx + widthInclMargin]);
#else
  // not initialized on purpose (speed)
  t_real w00;
  t_real w01;
  t_real w10;
  t_real w11;
  if (isValidIndex(ImgWidth-1,ImgHeight-1,x_int,y_int) && isValidIndex(ImgWidth,ImgHeight,x_int+1,y_int+1)){

      w00 = static_cast<t_real>(T[x_int   + yIdx   ]);
      w01 = static_cast<t_real>(T[x_int   + yIdx + widthInclMargin ]);
      w10 = static_cast<t_real>(T[x_int+1 + yIdx   ]);
      w11 = static_cast<t_real>(T[x_int+1 + yIdx + widthInclMargin]);
    }
    else{
      w00 = isValidIndex(ImgWidth, ImgHeight, x_int, y_int)
      ? static_cast<t_real>(T[x_int + yIdx])
      : 0;

      w01 = isValidIndex(ImgWidth, ImgHeight, x_int, y_int+1)
      ? static_cast<t_real>(T[x_int + yIdx + widthInclMargin])
      : 0;

      w10 = isValidIndex(ImgWidth, ImgHeight, x_int+1, y_int)
      ? static_cast<t_real>(T[(x_int+1)+yIdx])
      : 0;

      w11 = isValidIndex(ImgWidth, ImgHeight, x_int+1, y_int+1)
      ? static_cast<t_real>(T[(x_int+1)+yIdx + widthInclMargin])
      : 0;

    }
#endif

#ifdef _DO_ERROR_CHECKS_
	const int minIdx = x_int + yIdx;
	const int maxIdx = x_int+1 + yIdx + widthInclMargin;
	if(minIdx<gMinAllowedIdx || maxIdx>gMaxAllowedIdx)
	{
		logout("ERROR: Accessing invalid pixels in template image.\n");
		exit(0);
	}
#endif

  // calculate remainder
  const t_real dx_1 = 1.0f-dx;
  const t_real dy_1 = 1.0f-dy;
  
  // calculate derivatives
  dTx = (dy_1*(-w00 + w10) + dy*(-w01 + w11)) / 255.0f;
  dTy = (dx_1*(-w00 + w01) + dx*(-w10 + w11)) / 255.0f;
  
  // calculate function value; "Horner Schema"-style
  return (dx_1*(dy_1*w00 + dy*w01) + dx*(dy_1*w10 + dy*w11)) / 255.0f;
}

#ifdef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
inline bool isValidIndex(int M1, int M2, int i, int j) {

  // slightly faster cascaded check
  if ((i >= 0) && (i < M1)){
    if ((j >= 0) && (j < M2)){
      return true;
    }
  }

  return false;
}
#endif
