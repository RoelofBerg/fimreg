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

// NOTE: The linker doesn't allow template compile type soecification in the .cpp file
//(Which would read like: template class CMatlabArrayFlex<emxArray_real32_T, float>;)
//So we clutter the implementation into the headerfile. (This is quite usual anyway ...)

#pragma once

#include "dspreg_common.h"
#include "dspreg_types.h"
#include "dspreg_emxutil.h"	//necessary because of the above mentioned compiler limitation

static const int32_t MAX_SIZE_ARRAY_SIZE = 2;		//support max. 2 dimenstions

/**
 * \brief Controls memory for an array used by Matlab.
 *
 */
template<class T, class U>
class CMatlabArrayFlex
{
public:
	CMatlabArrayFlex(U* StaticBuffer, const uint32_t StaticBufferLen, const uint32_t ItemCount)
	: m_pStaticBuffer(StaticBuffer),
	  m_iStaticBufferLen(StaticBufferLen)
	{
		CMatlabArrayFlex<T,U>::InitializeArray(StaticBuffer, &ItemCount, 1);
	}

	CMatlabArrayFlex(U* StaticBuffer, const uint32_t StaticBufferLen, const uint32_t* ItemCount, const uint32_t NumDimensions)
	: m_pStaticBuffer(StaticBuffer),
	  m_iStaticBufferLen(StaticBufferLen)
	{
		CMatlabArrayFlex<T,U>::InitializeArray(StaticBuffer, ItemCount, NumDimensions);
	}

	void Reinitialize(const uint32_t ItemCount)
	{
		Reinitialize(&ItemCount, 1);

	}

	void Reinitialize(const uint32_t* ItemCount, const uint32_t NumDimensions)
	{
		InitializeArray(m_pStaticBuffer, ItemCount, NumDimensions);
	}

	virtual ~CMatlabArrayFlex()
	{
		logout("ERROR: Trying to deallocate matlab array. We allocate at boot time and shut down using poweroff.\n");
		exit(0);
	}

	U* GetCMemoryArrayPtr()
	{
		//return m_MatlabArray.EmxArray.data;
		return m_MatlabArray.data;
	}

	T* GetMatlabArrayPtr()
	{
		//return &(m_MatlabArray.EmxArray);
		return &m_MatlabArray;
	}

private:
	T m_MatlabArray;
	U* m_pStaticBuffer;
	const uint32_t m_iStaticBufferLen;
	int32_t m_pSizeArray[MAX_SIZE_ARRAY_SIZE];

	void InitializeArray(U* StaticBuffer, const uint32_t* ItemCount, const uint32_t NumDimensions)
	{
		#ifdef _DO_ERROR_CHECKS_
		if(MAX_SIZE_ARRAY_SIZE < NumDimensions)
		{
			printf("ERROR: Only matlab arrays of %d dimensions or less are supported (requested: %d)\n", MAX_SIZE_ARRAY_SIZE, NumDimensions);
		}
		#endif

		//Init buffer
		m_MatlabArray.data = m_pStaticBuffer;
		m_MatlabArray.numDimensions = NumDimensions;
		m_MatlabArray.size = m_pSizeArray;
		m_MatlabArray.allocatedSize = 0;
		m_MatlabArray.canFreeData = TRUE;

		uint32_t uiCapacity=1;
		for(uint32_t i=0; i<NumDimensions; i++)
		{
			//ToDo: multiply all size values and check that the result is < MAX_SIZE_ARRAY_SIZE
			m_MatlabArray.size[i] = (int32_T)ItemCount[i];
			uiCapacity *= ItemCount[i];
		}
		emxEnsureCapacity((emxArray__common *)(&m_MatlabArray), 0, (int32_T)sizeof(U));
	}
};


/**
* A Matlab vector containing float values
*/
struct emxArray_real32_T;
typedef CMatlabArrayFlex<emxArray_real32_T, float> TMatlabArray_Float;
typedef TMatlabArray_Float TMatlabArray_Reg_Real;

/**
* A Matlab vector containing uint32 values
*/
struct emxArray_uint32_T;
typedef CMatlabArrayFlex<emxArray_uint32_T, uint32_t> TMatlabArray_UInt32;
