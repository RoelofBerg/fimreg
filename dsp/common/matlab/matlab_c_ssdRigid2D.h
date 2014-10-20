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


#ifndef RIGID_REGISTRATION_MATRIX_FREE_SSD
#define RIGID_REGISTRATION_MATRIX_FREE_SSD

typedef float t_real;
#if defined(_MSC_VER)
typedef unsigned __int8 uint8_t;
#else
#include <stdint.h>
#endif
typedef uint8_t t_pixel;

#include "math.h"

// #############################################################################
// Mark: -
// Mark: Function declarations
// #############################################################################

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Function value and derivative computations
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Computes the SSD function value without derivatives
// params w = [rotation translation_x translation_y]
t_real matlab_c_ssdRigid2D(t_pixel* dataR, int mR[2], t_real omegaR[4],
                            t_pixel* dataT, int mT[2], t_real omegaT[4],
                            const t_real w[3], const unsigned int MarginAddon[3],
							const t_real DSPRange[4],
							const unsigned int i_from, const unsigned int i_to);

// Computes the SSD function value and derivatives
// params w = [rotation translation_x translation_y]
t_real matlab_c_ssdRigid2D(t_pixel* dataR, int mR[2], t_real omegaR[4],
                                          t_pixel* dataT, int mT[2], t_real omegaT[4],
                                          const t_real w[3],
										  const unsigned int MarginAddon[3],
										  const t_real DSPRange[4],
										  const unsigned int i_from, const unsigned int i_to,
										  t_real grad[3], t_real H_rigid[9]);

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Routines for bilinear interpolation with and without derivatives
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Interpolates the image T at given voxel coordinates
inline t_real linearInterPoint2D(const t_pixel* T, const t_real x_voxel, const t_real y_voxel, const int widthInclMargin
#ifdef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
		,const int ImgWidth, const int ImgHeight
#endif
);

// Interpolates the image T at given voxel coordinates and computes derivatives
inline t_real linearInterPoint2D(const t_pixel* T, const t_real x_voxel, const t_real y_voxel, t_real& dTx, t_real& dTy, const int widthInclMargin
#ifdef _TEST_DIRICHLET_AS_IF_THERE_WAS_NO_PADDING_
		,const int ImgWidth, const int ImgHeight
#endif
);

#endif
