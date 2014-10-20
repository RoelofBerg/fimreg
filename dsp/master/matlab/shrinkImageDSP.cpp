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
 * Code generation for function 'shrinkImageDSP'
 *
 * C source code generated on: Mon Jul 02 10:11:21 2012
 *
 */

#include "stdafx.h"

/* Include files */
#include "shrinkImageDSP.h"

/* Custom Source Code */
#include "pseudo_stdafx.h"             //precompiled header not possible because of include position of matlab

#include "shrinkImage_on_core.h"			//One is allowed to call directly shrinkImage_on_core() here (but must omit the CacheInvalidate here then) when the image is so small that a distributed calculation makes no sence.

#include "mcip_process.h"	//register_shrink and start_shrink functions (IPC distributed function calls)

#include <xdc/cfg/global.h>

/* Type Definitions */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */

/* Function Definitions */

/**
 * Calculate values common to cacheWriteBackSubImage() and shrinkImageDSP()
 */
void calcCommonValues(const uint32_T ImgDim[4], uint32_T SubArea[4], uint32_T& xWidth, uint32_T& yEvenOdd, uint32_T& yStart, uint32_T& yEnd)
{
	xWidth = ImgDim[1] - ImgDim[0] + 1U;
	yEvenOdd = ((SubArea[2]-1) & 0x1);	//todo: Maybe unnecessary on the dsp when y+=2 loops are equally fast as y++
	yStart = ((SubArea[2]-1)>>1);
	yEnd = int32_T((SubArea[3]-1)>>1)+1;	//todo: why int ? Is this a leftover of PC the OpenMP implementation (where uint wasn't allowed in some cases)
}

#if 0
/**
 * Call this method on the main core for preparation of the later shrink process. CacheWB the interesting parts of an image.
 * (The parts which are not covered by an already cacheWBed black margin)
 */
void cacheWriteBackSubImage(const uint8_T *Img, const uint32_T ImgDim[4], uint32_T SubArea[4])
{
	uint32_T xWidth;
	uint32_T yEvenOdd;
	uint32_T yStart;
	uint32_T yEnd;

	calcCommonValues(ImgDim, SubArea, xWidth, yEvenOdd, yStart, yEnd);
	CacheWriteBackToBeShrinkedData(Img, yStart, yEnd, yEvenOdd, xWidth);
}
#endif

/**
 * Shrinks image data, handles CacheInval and CacheWB correctly.
 * (Also regarding black margins, which means to CacheInval regions that will (not) be written to).
 */
void shrinkImageDSP(const uint8_T *Img, const uint32_T ImgDim[4], uint32_T SubArea[4], uint32_T NumberOfCores, uint8_T *ImgSmall)
{
	uint32_T nc_xWidth;		//Used to force const below (which might give the compiler a hint for a better optimization of this very time critical code)
	uint32_T nc_yEvenOdd;
	uint32_T nc_yStart;
	uint32_T nc_yEnd;
	calcCommonValues(ImgDim, SubArea, nc_xWidth, nc_yEvenOdd, nc_yStart, nc_yEnd);

	const uint32_T xWidth = nc_xWidth;
	const uint32_T xWidthSmall = (uint32_T)(real32_T)ceil((real32_T)xWidth / 2.0F);
	const uint32_T yHeight = ImgDim[3] - ImgDim[2] + 1;

	const uint32_T yEvenOdd = nc_yEvenOdd;	//todo: Maybe unnecessary on the dsp when y+=2 loops are equally fast as y++

	//Shrink image in parallel. (All pixels that are not on an odd border line)

	//First prepare data to be processed by the IPC mechanism
	const uint32_T yStart = nc_yStart;
	const uint32_T yEnd = nc_yEnd;
	const uint32_T yRange = yEnd-yStart;	//Does this always equal yHeight ? Then we could use yHeight instead.
	const uint32_T yCoreStep = yRange/NumberOfCores;
	uint32_T yStartPart=0;
	uint32_T yEnd_recorded_for_the_last_code = 0;	//Remember the state of yEnd that is sent to core 1 for cache invalidation. (The last iteration is on core 0, we need yEnd in the iteration before).

#warn find best value again after optimizing cache_wb ...
	//RBE TODO: This kind of decision wcether a distributed calculation makes sense could also be useful for the calculation of jacobian and hessian ...
	const uint32_T cuMaxImgSizeForDistrComp=256;	//We just measured this out (128, 256, 512 ...), could have been more measurements ...
	bool bShrinkRemote=(yEnd-yStart)>cuMaxImgSizeForDistrComp;		//At small image sizes a distributed calculation makes no sense at it costs too much ...

	uint32_T yEndPart=0;
	for(uint32_T i=0; i<NumberOfCores; i++)
	{
		if((NumberOfCores-1) == i)	//last iteration
		{
			yEnd_recorded_for_the_last_code = yEndPart;  //remember yEnd of the iteration before the last iteration for cache invalidation
			yEndPart = yEnd;		//Don't use more or less lines than yEnd in the last iteration
		}
		else
		{
			yEndPart = yStartPart+yCoreStep;
			if(0 != (yCoreStep&0x1))	//When yCoreStep is an odd number we want to give alternating, even line amounts to the cores (e.g. 31: 30,32,30,32 ...)
			{
				yEndPart += (int32_T)((i&0x1)<<1)-1;		//-1, +1, -1, +1, -1, +1 ...
			}
		}

		if(bShrinkRemote)
		{
			//Note: Will call cacheinval and cachewriteback internally
			register_shrink_on_core(Img, SubArea, yStartPart, yEndPart, yEvenOdd, yHeight, xWidth, xWidthSmall, ImgSmall, (CORE_AMOUNT-1)-i /*begin at the last core*/);
		}
		else
		{
			//rbe todo: in this case we don't need to split up the calls in a for-loop ...
			//Note: Will call cachewriteback internally
			shrinkImage_on_core(Img, SubArea, yStartPart, yEndPart, yEvenOdd, yHeight, xWidth, xWidthSmall, ImgSmall);
		}
		yStartPart = yEndPart;
	}

	if(bShrinkRemote)
	{
		//Send IPC commands (rbe todo: would it be faster to call Core 1-7 over IPC but to call Core 0 directly (like above when bShrinkRemote is false))
		start_shrink_on_all_cores(NumberOfCores);

		//CacheInval data processed by the subcores here before continuing to process it below ... (or during the ssd/jac/hess operation, there we will need it too)
		//RBE Todo: IPC overhead can be limited when one core processes all iterations at once. (Now we call each for new for every iteration.)
		//          This can be archieved by changing register_shrink_on_core() to record several iterations. However this might affect cache
		//          invalidation (think carefully).
		CacheInvalReadyShrinkedData(ImgSmall, yStart, yEnd_recorded_for_the_last_code, xWidthSmall);

		//RBE TODO: If the above mentioned refactoring will not be made:
		//If the code below wouldn't affect the shrinked image borders (that is when the code below will be moved to shrinkImage_on_core.cpp)
		//then the cores have to cache-invalidate only at the first iteration ! At the following iterations the data is still valid (and
		//has only to CacheWBed but not to CacheInvalEd ...). Could save significant time because this data will definitely be USED during
		//the next runs ...
		//Steps: A) Move the code below to shrinkImage_on_core.cpp to be executed in parallel. B) Call CacheInvalShrinkData() only for the first
		//iteration (and after memory-zeroization on all cores also ... this would come into play then ...).
	}

	//Shrink image border lines if the line (right or down) is on an odd number of pixels
	//todo: the for loops below can also be parallelized
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
