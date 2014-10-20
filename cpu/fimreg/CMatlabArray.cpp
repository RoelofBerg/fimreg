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
#include "fimreg_emxutil.h"

/**
* Ctor for ondimensional arrays
*/
template<class T, class U>
CMatlabArray<T,U>::CMatlabArray(const uint32_t ItemCount)
{
	CMatlabArray<T,U>::InitializeArray(&ItemCount, 1);
}

/**
* Ctor for twodimensional arrays
*/
template<class T, class U>
CMatlabArray<T,U>::CMatlabArray(const uint32_t* ItemCount, const uint32_t NumDimensions)
{
	CMatlabArray<T,U>::InitializeArray(ItemCount, NumDimensions);
}

/**
* Common code for all ctors
*/
template<class T, class U>
void CMatlabArray<T,U>::InitializeArray(const uint32_t* ItemCount, const uint32_t NumDimensions)
{
	//Allocate memory
	EmxInitArray(NumDimensions);
	uint32_t uiCapacity=1;
	for(uint32_t i=0; i<NumDimensions; i++)
	{
		m_MatlabArray->size[i] = (int32_T)ItemCount[i];
		uiCapacity *= ItemCount[i];
	}
	emxEnsureCapacity((emxArray__common *)m_MatlabArray, 0, (int32_T)sizeof(U));
}

#ifdef _DOUBLE_PRECISION_
//Matlab coder generates only code for used types, hence a compiler switch must deactivate unused types

/**
* Destructor for type double
*/
template<>
CMatlabArray<emxArray_real64_T, double>::~CMatlabArray()
{
	//Free memory
	emxFree_real64_T(&m_MatlabArray);
	m_MatlabArray = NULL;
}

#else

/**
* Destructor for type float
*/
template<>
CMatlabArray<emxArray_real32_T, float>::~CMatlabArray()
{
	//Free memory
	emxFree_real32_T(&m_MatlabArray);
	m_MatlabArray = NULL;
}

#endif //_DOUBLE_PRECISION_

/**
* Destructor for type uint32_t
*/
template<>
CMatlabArray<emxArray_uint32_T, uint32_t>::~CMatlabArray()
{
	//Free memory
	emxFree_uint32_T(&m_MatlabArray);
	m_MatlabArray = NULL;
}

/**
* Destructor for type uint8_t
*/
template<>
CMatlabArray<emxArray_uint8_T, uint8_t>::~CMatlabArray()
{
	//Free memory
	emxFree_uint8_T(&m_MatlabArray);
	m_MatlabArray = NULL;
}

#ifdef _DOUBLE_PRECISION_
//Matlab coder generates only code for used types, hence a compiler switch must deactivate unused types

/**
* Initialization for type double
*/
void template<>
CMatlabArray<emxArray_real64_T, double>::EmxInitArray(const uint32_t NumDimensions)
{
	emxInit_real64_T(&m_MatlabArray, NumDimensions);
}

#else

/**
* Initialization for type float
*/
template<>
void CMatlabArray<emxArray_real32_T, float>::EmxInitArray(const uint32_t NumDimensions)
{
	emxInit_real32_T(&m_MatlabArray, NumDimensions);
}

#endif //_DOUBLE_PRECISION_

/**
* Initialization for type uint32
*/
template<>
void CMatlabArray<emxArray_uint32_T, uint32_t>::EmxInitArray(const uint32_t NumDimensions)
{
	emxInit_uint32_T(&m_MatlabArray, NumDimensions);
}

/**
* Initialization for type uint8
*/
template<>
void CMatlabArray<emxArray_uint8_T, uint8_t>::EmxInitArray(const uint32_t NumDimensions)
{
	emxInit_uint8_T(&m_MatlabArray, NumDimensions);
}


/**
* Access matlab array abstraction (e.g. for passing to a matlab coder generated function)
*/
template<class T, class U>
T* CMatlabArray<T,U>::GetMatlabArrayPtr()
{
	return m_MatlabArray;
}

/**
* Access pointer to c-style array in memory for direct data access
*/
template<class T, class U>
U* CMatlabArray<T,U>::GetCMemoryArrayPtr()
{
	return m_MatlabArray->data;
}


//We must tell the compiler about all expectable instance types when a template is declared in a .cpp file
#ifdef _DOUBLE_PRECISION_
	//Matlab generates only code for used types, hence a compiler switch must deactivate unused types
	template class CMatlabArray<emxArray_real64_T, double>; 
#else
	template class CMatlabArray<emxArray_real32_T, float>; 
#endif //_DOUBLE_PRECISION_
template class CMatlabArray<emxArray_uint32_T, uint32_t>;
template class CMatlabArray<emxArray_uint8_T, uint8_t>;


