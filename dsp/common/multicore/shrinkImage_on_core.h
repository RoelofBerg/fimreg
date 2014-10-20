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
 * shrinkImage_on_core.h
 */

#ifndef SHRINKIMAGECORE_H_
#define SHRINKIMAGECORE_H_

#ifdef __cplusplus
extern "C" {
#endif
void shrinkImage_on_core(const uint8_T *Img, const uint32_T SubArea[4], const uint32_t yStart, const uint32_t yEnd,
		const uint32_t yEvenOdd, const uint32_t yHeight, const uint32_t xWidth, const uint32_t xWidthSmall, uint8_T *ImgSmall);
#ifdef __cplusplus
}
#endif

#if 0
#ifdef __cplusplus
extern "C" {
#endif
void CacheWriteBackToBeShrinkedData(const uint8_T *Img, const uint32_t yStart, const uint32_t yEnd, const uint32_t yEvenOdd, const uint32_t xWidth);
#ifdef __cplusplus
}
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
void CacheInvalReadyShrinkedData(const uint8_T *ImgSmall, const uint32_t yStart, const uint32_t yEnd, const uint32_t xWidthSmall);
#ifdef __cplusplus
}
#endif

#endif /* SHRINKIMAGECORE_H_ */
