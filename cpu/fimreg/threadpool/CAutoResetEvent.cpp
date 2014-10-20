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
#include "CAutoResetEvent.h"

#ifdef USE_P_THREAD

AutoResetEvent::AutoResetEvent(bool InitialState)
: flag_(InitialState)
{
  pthread_mutex_init(&protect_, NULL);
  pthread_cond_init(&signal_, NULL);
}

AutoResetEvent::~AutoResetEvent()
{
  pthread_mutex_destroy(&protect_);
  pthread_cond_destroy(&signal_);
}

void AutoResetEvent::Signal()
{
  pthread_mutex_lock(&protect_);
  flag_ = true;
  pthread_mutex_unlock(&protect_);
  pthread_cond_signal(&signal_);
}

/*
void AutoResetEvent::Reset()
{
  pthread_mutex_lock(&protect_);
  flag_ = false;
  pthread_mutex_unlock(&protect_);
}
*/

void AutoResetEvent::Wait()
{
  pthread_mutex_lock(&protect_);
  while( !flag_ ) // prevent spurious wakeups from doing harm
    pthread_cond_wait(&signal_, &protect_);
  flag_ = false; // waiting resets the flag
  pthread_mutex_unlock(&protect_);
}

#else

CAutoResetEvent::CAutoResetEvent(bool InitialState)
{
	mEventHandle = CreateEvent( 
        NULL,               // default security attributes
        FALSE,              // manual-reset event
        InitialState ? TRUE : FALSE,              // initial state is nonsignaled
        NULL				// object name
        ); 

    if (mEventHandle == NULL) 
    { 
		throw 0;
    }
}

CAutoResetEvent::~CAutoResetEvent()
{
	CloseHandle(mEventHandle);
}

void CAutoResetEvent::Wait()
{
	WaitForSingleObject(mEventHandle, INFINITE);
}

void CAutoResetEvent::Signal()
{
	SetEvent(mEventHandle);
}

void CAutoResetEvent::Reset()
{
	ResetEvent(mEventHandle);
}

#endif