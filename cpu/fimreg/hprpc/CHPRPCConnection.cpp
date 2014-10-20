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

#include "CHPRPCConnection.h"
#include "CDSPC8681.h"
#include "CBufferedWriter.h"

//Initialize static members
CDSPC8681 CHPRPCConnection::mDSPC8681;

CHPRPCConnection::CHPRPCConnection(uint32_t DSPIndex, uint32_t TotalDSPAmount)
: m_uDSPIndex(DSPIndex),
  moPCIEConnection(DSPIndex, TotalDSPAmount, mDSPC8681)
{
}

CHPRPCConnection::~CHPRPCConnection(void)
{
}

CBufferedWriter& CHPRPCConnection::GetBufferedWriter()
{
	return moPCIEConnection.GetBufferedWriter();
}

/**
* Start over on an existing connection (call this between several subsequent registrations !)
*/
void CHPRPCConnection::ReinitializeHPRPC()
{
	moPCIEConnection.ClearBufferedWriter();
}

/**
* Open PCIe for the whole application
* STATIC / Called directly from main for application init/deinit (and only once allthough there might be several instances of this class ...)
*/
bool CHPRPCConnection::InitPCIe()
{
	return mDSPC8681.Initialize();
}


/**
* Close PCIe for the whole application
* STATIC / Called directly from main for application init/deinit (and only once allthough there might be several instances of this class ...)
*/
void CHPRPCConnection::ShutdownPCIe()
{
	mDSPC8681.Shutdown();
}

uint32_t CHPRPCConnection::GetDSPIndex()
{
	return m_uDSPIndex;
}

/**
* Receive a HPRPC command (blocking). The current implementation is blocking. (A return value of
* false would indicate that no data is available yet in a nonblocking version. But
* as said before, currently this method blocks, so the caller will allways receive
* a true.)
*/
void CHPRPCConnection::RcvHPRPCCommand(uint8_t*& Buffer, uint32_t& BufferLen)
{
	//Indicate nothing received in case of an error
	Buffer=NULL;
	BufferLen=0;

	uint32_t iDataAvailable=0;
	//Receive from PCIe (currently blocking)
	moPCIEConnection.Receive(mpInputBuffer, mciInputBufferLen, iDataAvailable);
		
	//Data available. Caller can process the receive buffer.
	Buffer=mpInputBuffer;
	BufferLen=iDataAvailable;
}

/**
* Connect to the target DSP.
*/
void CHPRPCConnection::ConnectToTarget()
{
	moPCIEConnection.Connect();
}

