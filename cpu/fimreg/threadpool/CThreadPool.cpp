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

//precompiled header (MUST BE THE FIRST ENTRY IN EVERY CPP FILE)
#include "stdafx.h"

#include "CThread.h"
#include "CThreadPool.h"

CThreadPool::CThreadPool()
{
	//Bind thread to a CPU core
	#ifdef ENFORCE_SINGLE_CORE	
	if(0 == SetThreadAffinityMask(GetCurrentThread(), 1))	//bind main thread to core 0
	//if(0 == SetThreadIdealProcessor(GetCurrentThread(), 0))
	{
		printf("Cannot set thread affinity of the main thread to core 0.\n");
		throw 0;
	}
	#endif

	for(int i=0; i<THREAD_POOL_SIZE; i++)
	{
		CreatePoolThread(i);
	}
}

CThreadPool::~CThreadPool()
{
	for(int i=0; i<THREAD_POOL_SIZE; i++)
	{
		Execute(i, 0, 0);	//Send exit signal
	}
}

void CThreadPool::Execute(uint32_t CoreNo, ThreadEntryFct lpStartAddress, void* lpParameter)
{
	if((CoreNo+1) > THREAD_POOL_SIZE)
	{
		//Execute the last thread directly (on the core which is calling)
		lpStartAddress(lpParameter);
	}
	else
	{
		CThread* pThread = &mThreadList[CoreNo];
		pThread->Execute(lpStartAddress, lpParameter);
	}
}

void CThreadPool::CreatePoolThread(uint32_t CoreNo)
{
	CThread* pThread = &mThreadList[CoreNo];

#ifdef USE_P_THREAD
	pthread_t threadHdl;
	int ReturnCode = pthread_create(&threadHdl, NULL, CThreadPool::ThreadEntry, (void*) pThread); 
    if(0 != ReturnCode) 
    { 
        printf("Error - pthread_create() return code: %d\n",ReturnCode); 
        exit(EXIT_FAILURE); 
    } 

#else

	DWORD threadID=0;
	HANDLE threadHdl = CreateThread( 
       NULL,                   // default security attributes
       0,                      // use default stack size  
       CThreadPool::ThreadEntry,			 // thread function name
       pThread,				   // argument to thread function 
       0,                      // use default creation flags 
       &threadID			   // returns the thread identifier 
	);

#endif
}

#ifdef USE_P_THREAD
void
#else
DWORD WINAPI 
#endif
CThreadPool::ThreadEntry(void* lpParam)
{
	//Rout call into instance.
	CThread* pInstance = (CThread*) lpParam;
	pInstance->ThreadFct();
	return 0;
}
