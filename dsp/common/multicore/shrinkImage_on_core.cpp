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

#include "stdafx.h"

/* Include files */
#include "rt_nonfinite.h"
#include "calcDSPLayout.h"
#include "jacobian_on_core.h"
#include "ssd_on_core.h"
#include "shrinkImage_on_core.h"
#include "dspreg_emxutil.h"
#include "dspreg_rtwutil.h"

#include <ti/sysbios/hal/Cache.h>
#include <ti/ipc/MultiProc.h>

/* Custom Source Code */
#include "pseudo_stdafx.h"             //precompiled header not possible because of include position of matlab
#include "cache_handling.h"

/* Type Definitions */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */

void CalcInputBufferPos(const uint8_T *Img, const uint32_t yStart, const uint32_t yEnd, const uint32_t yEvenOdd, const uint32_t xWidth, uint8_T*& buf_start, uint32_T& buf_size);
void CalcOutputBufferPos(const uint8_T *ImgSmall, const uint32_t yStart, const uint32_t yEnd, const uint32_t xWidthSmall, uint8_T*& buf_start, uint32_T& buf_size);
void CacheInvalShrinkData(const uint8_T *Img, const uint8_T *ImgSmall, const uint32_t yStart, const uint32_t yEnd, const uint32_t yEvenOdd, const uint32_t xWidth, const uint32_t xWidthSmall);
void CacheWriteBackShrinkData(const uint8_T *ImgSmall, const uint32_t yStart, const uint32_t yEnd, const uint32_t xWidthSmall);

/* Function Definitions */

void shrinkImage_on_core(const uint8_T *Img, const uint32_T SubArea[4], const uint32_t yStart, const uint32_t yEnd,
		const uint32_t yEvenOdd, const uint32_t yHeight, const uint32_t xWidth, const uint32_t xWidthSmall, uint8_T *ImgSmall)
{
  /* ToDo: When SubArea is > ImgDim (in some way), only the first for-loop is */
  /* necessary (without the -1 in the end-index) because the margin can be */
  /* kind of re-used ... */
  /* (However, might not save speed. E.g. at 4096x4096 the for loops below are */
  /* less than 5% of all pixels ...) */
  #ifdef _TRACE_SHRINK_
	  logout("SHRINK IMAGE ON CORE. Y from %d to %d\n", yStart, yEnd);

    bool bFirstShrink=true;
    uint32_T uLastPtr=0;
  #endif

  //Invalidate cache
   CacheInvalShrinkData(Img, ImgSmall, yStart, yEnd, yEvenOdd, xWidth, xWidthSmall);	//skipped internally on main core

  for (int32_T y = yStart; y < yEnd; y++) {
    const uint32_T iy = (y<<1) + yEvenOdd;
	const uint32_T pOut = y * xWidthSmall;
    for (uint32_T x = (SubArea[0]-1); x < (SubArea[1]-1); x += 2U) {
	  const uint32_T pIn1 = iy * xWidth + x;
	  const uint32_T pIn2 = pIn1 + xWidth;
      const uint32_T A =
				  (uint32_T)Img[pIn1]
				+ (uint32_T)Img[pIn1 + 1]
				+ (uint32_T)Img[pIn2]
				+ (uint32_T)Img[pIn2 + 1];

	  #ifdef _TRACE_SHRINK_
      if(bFirstShrink)
      {
    	  logout("PYRAMID START READ PTR 0x%x\n", &Img[pIn1]);
    	  bFirstShrink=false;
      }
      else
      {
    	  uLastPtr = (uint32_T)(&Img[pIn2 + 1]);
      }
	  #endif

      ImgSmall[pOut + (x>>1)] = (uint8_T)(A>>2);
    }
  }

  //Cache write back (also on/from main core)
  CacheWriteBackShrinkData(ImgSmall, yStart, yEnd, xWidthSmall);

  #ifdef _TRACE_SHRINK_
  logout("PYRAMID START READ PTR 0x%x\n", uLastPtr);
  #endif

}

void CalcInputBufferPos(const uint8_T *Img, const uint32_t yStart, const uint32_t yEnd, const uint32_t yEvenOdd, const uint32_t xWidth, uint8_T*& buf_start, uint32_T& buf_size)
{
	//Input start
    const uint32_T iyS = (yStart<<1) + yEvenOdd;
    const uint32_T pInS = iyS * xWidth;
    buf_start = (uint8_T*) &Img[pInS];

    //Input end
    const uint32_T iyE = (yEnd<<1) + yEvenOdd;		//We waste a few bytes at the end of each (and also the last) line, not important
    const uint32_T pInE = iyE * xWidth;
    uint8_T* buf_end = (uint8_T*) &Img[pInE];

    buf_size = buf_end - buf_start;
}

void CalcOutputBufferPos(const uint8_T *ImgSmall, const uint32_t yStart, const uint32_t yEnd, const uint32_t xWidthSmall, uint8_T*& buf_start, uint32_T& buf_size)
{
    //Output start
    const uint32_T pOutS = yStart * xWidthSmall;
    buf_start = (uint8_T*) &ImgSmall[pOutS];

    //Output end
    const uint32_T pOutE = yEnd * xWidthSmall;		//We waste a few bytes at the end of each (and also the last) line, not important.
    uint8_T* buf_end = (uint8_T*) &ImgSmall[pOutE];

    buf_size = buf_end - buf_start;
}

void CacheInvalShrinkData(const uint8_T *Img, const uint8_T *ImgSmall, const uint32_t yStart, const uint32_t yEnd, const uint32_t yEvenOdd, const uint32_t xWidth, const uint32_t xWidthSmall)
{
	if (0 != MultiProc_self())	//skip on main core
	{
		#if 0

		//Invalidate input image data
		uint8_T* in_buf_start;
		uint32_T in_buf_size;
		CalcInputBufferPos(Img, yStart, yEnd, yEvenOdd, xWidth, in_buf_start, in_buf_size);
		Cache_inv((xdc_Ptr*) in_buf_start, in_buf_size, Cache_Type_ALL, CACHE_BLOCKING);		//Blocking because we will process the data immediately

		#endif

		//Invalidate parts of the output cache, because the main core zeroed memory after boot (or after the last calculation was finished when the DSP was idle)
		//In CacheWriteBackShrinkData() we push all image data, the data we wrote to and also the end of the lines which we didn't touch here. Because we didn't
		//touch the end of the lines (and left it black) we have to cacheinval it here to be sure its blackness is known to this core).
		/* Should be unnecessary since I introduced CacheInvalShrinkData()
		uint8_T* out_buf_start;
		uint32_T out_buf_size;
		CalcOutputBufferPos(ImgSmall, yStart, yEnd, xWidthSmall, out_buf_start, out_buf_size);
		Cache_inv((xdc_Ptr*) out_buf_start, out_buf_size, Cache_Type_ALL, CACHE_BLOCKING);		//Blocking because we will process the data immediately
		*/
	}
}

/**
 * Public, called from shrinkImageDSP (on the main core) after entering shrinkImage_on_core()
 */
void CacheInvalReadyShrinkedData(const uint8_T *ImgSmall, const uint32_t yStart, const uint32_t yEnd, const uint32_t xWidthSmall)
{
#ifdef _NO_IPC_TEST_
	if (0 == MultiProc_self())	//skip on main core
		return;
#else
	//ASSERT(0 == MultiProc_self());
#endif


	#if 0
	//Invalidate the output cache, we need the data the other cores wrote to.
	uint8_T* out_buf_start;
	uint32_T out_buf_size;
	CalcOutputBufferPos(ImgSmall, yStart, yEnd, xWidthSmall, out_buf_start, out_buf_size);
	Cache_inv((xdc_Ptr*) out_buf_start, out_buf_size, Cache_Type_ALL, CACHE_BLOCKING);		//Blocking because we will process the data immediately
	#endif
}

void CacheWriteBackShrinkData(const uint8_T *ImgSmall, const uint32_t yStart, const uint32_t yEnd, const uint32_t xWidthSmall)
{
	//Cache write back output cache, the memory this core (and only this core) has written to (also cache write back on main core)
	uint8_T* out_buf_start;
	uint32_T out_buf_size;
	CalcOutputBufferPos(ImgSmall, yStart, yEnd, xWidthSmall, out_buf_start, out_buf_size);
	Cache_wb((xdc_Ptr*) out_buf_start, out_buf_size, Cache_Type_ALL, CACHE_BLOCKING);	//Blocking because the main core will process the image edges after we're finished here
}

#if 0
/**
 * Public, called from shrinkImageDSP (on the main core) before entering shrinkImage_on_core()
 */
void CacheWriteBackToBeShrinkedData(const uint8_T *Img, const uint32_t yStart, const uint32_t yEnd, const uint32_t yEvenOdd, const uint32_t xWidth)
{
	//ASSERT(0 == MultiProc_self());

	//Cache write back the data on core 0 that other cores will need later (and pull by CacheInvalShrinkData())
	uint8_T* in_buf_start;
	uint32_T in_buf_size;
	CalcInputBufferPos(Img, yStart, yEnd, yEvenOdd, xWidth, in_buf_start, in_buf_size);
	Cache_wb((xdc_Ptr*) in_buf_start, in_buf_size, Cache_Type_ALL, CACHE_BLOCKING);		//Blocking because we will process the data immediately
}
#endif
