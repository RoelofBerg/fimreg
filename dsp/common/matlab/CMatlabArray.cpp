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

#include "stdafx.h"	//Precompiled headerfile, MUST be first entry in .cpp file
#include "CMatlabArray.h"
#include "dspreg_emxAPI.h"

// !!!!!!!!!!!!!!!!!!!
//TODO: Use CMatlabArrayFlex for uint8_t and delete this class !!!
// !!!!!!!!!!!!!!!!!!!

/**
* Constructor for type uint8_t
*/
CMatlabArray::CMatlabArray(uint8_t* StaticBuffer, uint32_t StaticBufferLen)
: m_pStaticBuffer(StaticBuffer),
  m_iStaticBufferLen(StaticBufferLen)
{
	//Assign 0-sized memory
	emxCreateWrapperND_uint8_T(&m_MatlabArray, m_pStaticBuffer, 0);

	//memset(m_pStaticBuffer, 0, StaticBufferLen);
}

/**
* Destructor for type uint8_t
*/
CMatlabArray::~CMatlabArray()
{
	//Free memory
	emxDestroyArray_uint8_T(&m_MatlabArray);
}

/**
 * We will not allocate memory on the target system (e.g. to avoid heap fragmentation as the system might run for years and
 * no MMU is present). Hence the operations of discarding an old and creating a new instance can be executed here. The result
 * is the same as if the caller would drop an old and create a new instance.
 */
void CMatlabArray::Reinitialize(uint32_t ItemCount)
{
	if(ItemCount>m_iStaticBufferLen)
	{

		logout("ERROR: Trying to allocate too much Matlab vector space (%u > %u).\n", ItemCount, m_iStaticBufferLen);
		logout("ERROR:  OutOfMemoryException()\n");
		exit(0);
	}

	//Assign memory (virtually, the underlying memory was statically allocated at boot time in the embedded version of the software)
	emxCreateWrapperND_uint8_T(&m_MatlabArray, m_pStaticBuffer, ItemCount);
}

/**
* Access matlab array abstraction (e.g. for passing to a matlab coder generated function)
*/
emxArray_uint8_T* CMatlabArray::GetMatlabArrayPtr()
{
	return &(m_MatlabArray.EmxArray);
}

/**
* Access pointer to c-style array in memory for direct data access
*/
uint8_t* CMatlabArray::GetCMemoryArrayPtr()
{
	return m_MatlabArray.EmxArray.data;
}

