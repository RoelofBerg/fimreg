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

#include "CHPRPCCommand.h"
#include "CHPRPCConnection.h"
#include "CBufferedWriter.h"

CHPRPCCommand::CHPRPCCommand(CHPRPCConnection& HPRPCConnection)
: mpPayloadHeader(0),
  m_HPRPCConnection(HPRPCConnection)
{

}

CHPRPCCommand::~CHPRPCCommand(void)
{
}

CHPRPCConnection& CHPRPCCommand::GetHPRPCConnection()
{
	return m_HPRPCConnection;
}

/**
* Wait until a response is received from the remote peer. Usually called after StartSendToOtherPeer() was called to all remote peers.
* This command will block. In this way several commands will be sent to the remote peers, processed by the remote peers and the answers
* will then be processed one by one.
*
* (Use this bocking command only if all answers have to be available for processing and if receiving the answers does take nearly no time.
* Under this circumstances one will not notice any significant performance impact by this partial blocking behavior.)
*/
void CHPRPCCommand::ReceiveResponse(uint8_t*& Buffer, uint32_t& Bufferlen
	#if defined(_TRACE_HPRPC_)
	, const string& CommandLogString
	#endif
	)
{
	#if defined(_TRACE_HPRPC_)
	const uint32_t DSPIdx = m_HPRPCConnection.GetDSPIndex();
	printf("[DSP_%u] Receiving answer for HPRPC command %s.\n", DSPIdx, CommandLogString.c_str());
	#endif
	m_HPRPCConnection.RcvHPRPCCommand(Buffer, Bufferlen);
	#if defined(_TRACE_HPRPC_)
	printf("[DSP_%u] Answer for HPRPC command %s received.\n", DSPIdx, CommandLogString.c_str());
	#endif
}

/**
* Send the HPRPC command to the remote peer. This command will block until sending
* has startet but not block until sending has finished. It will return to the
* caller while sending is still in progress.
*
* DO NOT DESTROY THE BUFFER BEFORE SENDING THE DATA WILL BE FINISHED !
*/
void CHPRPCCommand::StartSendToOtherPeer(
	#if defined(_TRACE_HPRPC_)
	const string& CommandLogString
	#endif
	)
{
	#ifdef _DO_ERROR_CHECKS_
	if(false == mCommandBuffer.IsABufferAssigned())
	{
		//We can only send commands that were created before using AllocateBuffer().
		//E.g. by a method like CHPRPCCommandStoreImg::AssignValuesToBeSent()
		throw InvalidBufferException();
	}
	#endif

	//Send data (async)
	#if defined(_TRACE_HPRPC_)
	const uint32_t DSPIdx = m_HPRPCConnection.GetDSPIndex();
	printf("[DSP_%u] Start sending HPRPC command '%s'.\n", DSPIdx, CommandLogString.c_str());
	#endif

	//Send main buffer (header and for small commands also the data)
	CBufferedWriter& oBufferedWriter = GetBufferedWriter();
	oBufferedWriter.Send();

	#if defined(_TRACE_HPRPC_)
	printf("[DSP_%u] Sending of HPRPC command '%s' is now in progress.\n", DSPIdx, CommandLogString.c_str());
	#endif
}

/**
* Return the begining of the payload (payload header if any or payload data).
* Throws an exception if no buffer was assigned yet.
* Before this method can succeed 
* AssignRcvBuffer() of one of the base classes must have been called (which
* will mean that either a new command to be send must have been generated or
* incoming data of a received command must have been assigned.)
*/
uint8_t* CHPRPCCommand::GetPayloadHeaderPtr()
{
	if(NULL == mpPayloadHeader)
	{
		throw InvalidBufferException();
	}

	return mpPayloadHeader;
}

/**
* Remember the begining of the payload inside the HPRPC command.
*/
void CHPRPCCommand::SetPayloadHeaderPtr(uint8_t* PayloadHeaderPtr)
{
	mpPayloadHeader = PayloadHeaderPtr;
}

CBufferedWriter& CHPRPCCommand::GetBufferedWriter()
{
	CBufferedWriter& oBufferedWriter = m_HPRPCConnection.GetBufferedWriter();
	return oBufferedWriter;
}
