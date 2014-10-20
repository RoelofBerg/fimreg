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

#include "stdafx.h"
#include "CThread.h"

uint32_t CThread::mThreadAmount(0);

CThread::CThread(void)
  : mFct(0),
	mParam(0),
	mAutoResetEvent(false)
{
	mThreadNo = mThreadAmount;
	mThreadAmount++;

	#ifdef _TRACE_THREADPOOL_
	printf("Starting thread %u with associated event '%s'.\n", mThreadNo, gEventNames[mThreadNo]);
	#endif
}


CThread::~CThread(void)
{
}

void CThread::Execute(ThreadEntryFct Fct, void* Param)
{
	mFct = Fct;
	mParam = Param;

	#ifdef _TRACE_THREADPOOL_
	printf("Wake up thread %u\n", mThreadNo);
	#endif

	//Signal event
	mAutoResetEvent.Signal();
}
    
void CThread::ThreadFct()
{
	//Bind thread to a CPU core
	#ifdef _TRACE_THREADPOOL_
	printf("Set thread affinity of the worker thread with the ID %u to core %u.\n", mThreadNo, (mThreadNo+1));
	#endif
	/*
	if(SetThreadAffinityMask(GetCurrentThread(), (1 << (mThreadNo+1))))	//+1 because the main thread is bound to core 0
	//if(0 == SetThreadIdealProcessor(GetCurrentThread(), (mThreadNo+1)))
	{
		printf("Cannot set thread affinity of the worker thread with the ID %u to core %u.\n", mThreadNo, (mThreadNo+1));
		throw 0;
	*/

	while(1)
	{
		#ifdef _TRACE_THREADPOOL_
		printf("Thread %u waiting ...\n", mThreadNo);
		#endif
		//Wait for event
		mAutoResetEvent.Wait();

		#ifdef _TRACE_THREADPOOL_
		printf("Thread %u woken up ...\n", mThreadNo);
		#endif
		//Exit signal ?
		if(0 == mFct)
		{
			#ifdef _TRACE_THREADPOOL_
			printf("Exiting Thread %u woken up ...\n", mThreadNo);
			#endif
			return;
		}

		#ifdef _TRACE_THREADPOOL_
		printf("Thread %u executing call ...\n", mThreadNo);
		#endif
		//Execute call 
		mFct(mParam);
	}
}
