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

/**
*  \brief Controls memory for an array used by Matlab.
*
*  \author Roelof Berg
*/

#pragma once

#include "fimreg_common.h"

/**
 * \brief Controls memory for an array used by Matlab.
 *
 * Use ctor and dtor to allocate and decallocate memory.
 * Use the get methods to access the arrays (either in c-style
 * or in Matlab style).
 *
 * First template patameter is the Matlab array type that will be
 * controlled. Second parameter is the C type controlled by the
 * matlab type.
 *
 * The casting operator was not overloaded for better code-readability.
 * (See "Clean Coder" from Robert C. Martin)
 */
template<class T, class U>
class CMatlabArray
{
public:
	CMatlabArray(const uint32_t ItemCount);
	CMatlabArray(const uint32_t* ItemCount, const uint32_t NumDimensions);
	virtual ~CMatlabArray();

	void InitializeArray(const uint32_t* ItemCount, const uint32_t NumDimensions);
	void EmxInitArray(const uint32_t NumDimensions);
	U* GetCMemoryArrayPtr();
	T* GetMatlabArrayPtr();

private:
	T* m_MatlabArray;
};


/**
* A Matlab vector containing double values
*/
struct emxArray_real64_T;
typedef CMatlabArray<emxArray_real64_T, double> TMatlabArray_Double;

/**
* A Matlab vector containing float values
*/
struct emxArray_real32_T;
typedef CMatlabArray<emxArray_real32_T, float> TMatlabArray_Float;

/**
* A Matlab vector containing uint32 values
*/
struct emxArray_uint32_T;
typedef CMatlabArray<emxArray_uint32_T, uint32_t> TMatlabArray_UInt32;

/**
* A Matlab vector containing bytes
*/
struct emxArray_uint8_T;
typedef CMatlabArray<emxArray_uint8_T, uint8_t> TMatlabArray_UInt8;

