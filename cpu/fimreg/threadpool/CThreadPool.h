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

#pragma once

#include "CThread.h"

const uint32_t THREAD_POOL_SIZE=3;

/**
* This is no perfect implmementation, just a quick hack for research purposes...
* (In an industrial project I would probably consider an 'active object' approach for this purpose).
* (Critical Sections are unnecessary because of the calling order.)
* (I didn't use boost because obtaining boost for an embedded target can be a lot of work, so I took BOOST only on places that are easy to refactor.).
*
* Example set THREAD_POOL_SIZE to 3 on a 4 core PC. Calls to the fourth thread will automatically be processed locally.
*/
class CThreadPool
{
public:
	CThreadPool::CThreadPool();
	virtual CThreadPool::~CThreadPool();

	void Execute(uint32_t CoreNo, ThreadEntryFct lpStartAddress,
		#ifndef USE_P_THREAD
		__deref __drv_aliasesMem 
		#endif
		void* lpParameter);

private:
	CThread mThreadList[THREAD_POOL_SIZE];

	void CreatePoolThread(uint32_t CoreNo);

	static
	#ifdef USE_P_THREAD
	void
	#else
	DWORD WINAPI 
	#endif
	ThreadEntry(LPVOID lpParam);

};