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

#include "dspreg_common.h"
#include "dspreg_types.h"
#include "CMatlabArrayFlex.h"

/**
 * \brief Controls memory for an array used by Matlab.
 *
 * Use ctor and dtor to allocate and decallocate memory.
 * Use the get methods to access the arrays (either in c-style
 * or in Matlab style).
 *
 * In the embedded version there's currently only one type supported.
 * Furthermore there is no memory allocated from the SYSBIOS operating
 * system. Instead (to prevent e.g. heap fragmentation when the target
 * is running for several years) a memory buffer allocated at boot time
 * will be used as underlying memory. However from outside the class
 * looks like allocating/deallocating memory (facade design pattern).
 *
 * The casting operator was not overloaded for better code-readability.
 * (See "Clean Coder" from Robert C. Martin)
 */
class CMatlabArray
{
public:

	// !!!!!!!!!!!!!!!!!!!
	//TODO: Use CMatlabArrayFlex for uint8_t and delete this class !!!
	// !!!!!!!!!!!!!!!!!!!

	CMatlabArray(uint8_t* StaticBuffer, uint32_t StaticBufferLen);
	virtual ~CMatlabArray();

	void Reinitialize(uint32_t ItemCount);

	uint8_t* GetCMemoryArrayPtr();
	emxArray_uint8_T* GetMatlabArrayPtr();

private:
	uint8_t* m_pStaticBuffer;
	uint32_t m_iStaticBufferLen;
	emxArrayWidh1DSize m_MatlabArray;
};

