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
 * ssd.cpp
 *
 */

/* Include files */
#include "calcDSPLayout.h"
#include "jacobian_on_core.h"
#include "ssd_on_core.h"

/* Custom Source Code */
#include "pseudo_stdafx.h"             //precompiled header not possible because of include position of matlab

#include "stdio.h"
#include "string.h"
#include "logout.h"
#include "matlab_c_ssdRigid2D.h"

real32_T ssd_on_core(const real32_T w[3], const uint32_T BoundBox[4], const uint32_T MarginAddon[3], const real32_T
             DSPRange[4], const uint8_T *Tvec, const uint32_T TOffset, const uint8_T
             *Rvec, const uint32_T ROffset, const uint32_T d, const uint32_T i_from, const uint32_T i_to)
{
    real32_T omegaR[4];
    real32_T omegaT[4];
    int32_T mR[2];
  	int32_T mT[2];

	real32_T SSD = 0.0F;

	#ifdef _TRACE_MC_
	logout("[ssd_%u] Calc SSD from %u to %u DSPRange[%f,%f,%f,%f]\n", i_from, i_from, i_to, (double)DSPRange[0], (double)DSPRange[1], (double)DSPRange[2], (double)DSPRange[3]);
	#endif

	  omegaR[0] = DSPRange[0]-0.5f;
	  omegaR[1] = DSPRange[1]+0.5f;
	  omegaR[2] = DSPRange[2]-0.5f;
	  omegaR[3] = DSPRange[3]+0.5f;
	  mR[0] = (int)(omegaR[1]-omegaR[0]);
	  mR[1] = (int)(omegaR[3]-omegaR[2]);

	  const real32_T shift = (d/2)+0.5f;
	  omegaT[0] = BoundBox[0] - shift-0.5f;
	  omegaT[1] = BoundBox[1] - shift+0.5f;
	  omegaT[2] = BoundBox[2] - shift-0.5f;
	  omegaT[3] = BoundBox[3] - shift+0.5f;
	  mT[0] = (int)(BoundBox[1]-BoundBox[0])+1;
	  mT[1] = (int)(BoundBox[3]-BoundBox[2])+1;

	  //debug
	  #ifdef _TRACE_OUTPUT_
		logout("d=%u, shift=%f\n", d, (double)shift);

		const uint8_T* p=&Tvec[TOffset];
		//uint8_t* p=mStoreImgRequest.GetTVec()->GetCMemoryArrayPtr();
		uint32_T iWidth = (BoundBox[1]+1)-BoundBox[0];
		uint32_T iHeight = (BoundBox[3]+1)-BoundBox[2];
		uint32_T iA=(iWidth+MarginAddon[0])*((iHeight/4)+MarginAddon[1])+(iWidth/4)-1;
		uint32_T iB=(iWidth+MarginAddon[0])*((iHeight/2)+MarginAddon[1])+(iWidth*3/4)-1;
		uint32_T iC=(iWidth+MarginAddon[0])*((iHeight*6/10)+MarginAddon[1])+(iWidth*8/10)-1;
		logout("f(0), f(1) ... f(0x%x), f(0x%x) ... f(0x%x), f(0x%x) ...\n", iA, iA+1, iB-1, iB);
		logout("T: %.2x %.2x ... %.2x %.2x ... %.2x %.2x ... %.2x %.2x\n",
				p[0], p[1], p[iA], p[iA+1], p[iB-1], p[iB], p[iC-1], p[iC]
				);

		p=&Rvec[ROffset];
		iWidth = uint32_T(DSPRange[1]-DSPRange[0]);
		iHeight = uint32_T(DSPRange[3]-DSPRange[2]);
		iA=iWidth*(iHeight/4)+(iWidth/4)-1;
		iB=iWidth*(iHeight/2)+(iWidth*3/4)-1;
		iC=iWidth*(iHeight*6/10)+(iWidth*8/10)-1;
		logout("R: %.2x %.2x ... %.2x %.2x ... %.2x %.2x ... %.2x %.2x\n",
				p[0], p[1], p[iA], p[iA+1], p[iB-1], p[iB], p[iC-1], p[iC]
				);
	  #endif

	SSD = matlab_c_ssdRigid2D((t_pixel*)(&Rvec[ROffset]), mR, omegaR,
							(t_pixel*)(&Tvec[TOffset]), mT, omegaT,
							&w[0], &MarginAddon[0],
							&DSPRange[0], i_from, i_to);

	#ifdef _TRACE_MC_
	logout("[ssd_%u] SSD = %f\n", i_from, (double)SSD);
	#endif

	return SSD;
}

/* End of code generation (ssd.cpp) */
