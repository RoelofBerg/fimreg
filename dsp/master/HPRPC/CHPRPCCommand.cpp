/****************************************************************************
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

//precompiled header (MUST BE THE FIRST ENTRY IN EVERY CPP FILE)
#include "stdafx.h"

#include "CHPRPCCommand.h"
#include "CHPRPCServer.h"

//Static buffer to avoid memory allocation
//L2 RAM not necessary as incoming calls use DDR3 heavily anyway and outgoing calls don't matter.
const unsigned int MAX_HPRPC_CMD=200;
uint8_t gCommandBuffer[MAX_HPRPC_CMD];	//Used for header-copies (e.g. to 8-byte-align an HPRPC header containing float-values) and for response buffers. Might fall apart anyway when PCIe communication will be steamlined (by avoiding memcpy-operations). This comes from the earlier ethernet-socket-implementation.

CHPRPCCommand::CHPRPCCommand(void)
: mpPayloadHeader(0),
  mBufferLen(0)
{
	mBuffer=gCommandBuffer;
}

CHPRPCCommand::~CHPRPCCommand(void)
{
	//Free memory if necessary
	FreeBufferIfAnyAssigned();
}

/**
* Allocate memory for assigning a new call. (NOTE: This is a virtual allocation on the target (not
* on the PC client version of this call), we use a static buffer on the target to prevent stuff like
* heap fragmentation.
* Also used for making a copy of the first arriving buffer containing the HPRPC command header
* when multi-frame-commands (like store image) are used.
*/
uint8_t* CHPRPCCommand::AllocateBuffer(const uint32_t BufferLen)
{
	#ifdef _DO_ERROR_CHECKS_
	if(BufferLen > MAX_HPRPC_CMD)
	{
		logout("ERROR: Trying to allocate too much HPRPC command buffer (%u > %u).\n", BufferLen, MAX_HPRPC_CMD);
		logout("ERROR:  OutOfMemoryException()\n");
		exit(0);
	}
	#endif

	//Virtually allocate new buffer (in fact we use a static buffer on the embedded DSP)
	mBufferLen = BufferLen;

	//Pass back pointer to caller
	return mBuffer;
}

/**
* When transmitting compressed data a derived class (e.g. CHPRPCRequestStoreImg) might allocate a bigger buffer than
* will actually be filled with data to be transmitted. The derived class can then tell the USED part (the part to be
* transmitted) by this method.
*/
void CHPRPCCommand::ChopTransmitBuffer(const uint32_t mChoppedBufferLen)
{
	mBufferLen = mChoppedBufferLen;
}

/**
* Free memory allocated in AllocateBuffer() (if any)
*/
void CHPRPCCommand::FreeBufferIfAnyAssigned()
{
	mBufferLen=0;
}

/**
* Send buffer to remote peer
*/
void CHPRPCCommand::SendToOtherPeer(CHPRPCServer& HPRPCServer)
{
	#ifdef _DO_ERROR_CHECKS_
	if(NULL == mBuffer)
	{
		//We can only send commands that were created before using AllocateBuffer().
		//E.g. by a method like CHPRPCCommandStoreImg::AssignValuesToBeSent()
		logout("ERROR:  InvalidBufferException(1)\n");
		exit(0);
	}
	#endif

	//Send data (async)
	HPRPCServer.SendDataFromTargetToPC(mBuffer, mBufferLen);
}

/**
* Return the begining of the payload (payload header if any or payload data).
* //throws an exception if no buffer was assigned yet.
* Before this method can succeed either AllocateBufferWithHPRPCHeader() or
* AssignRcvBuffer() of one of the base classes must have been called (which
* will mean that either a new command to be send must have been generated or
* incoming data of a received command must have been assigned.)
*/
uint8_t* CHPRPCCommand::GetPayloadHeaderPtr()
{
	#ifdef _DO_ERROR_CHECKS_
	if(NULL == mpPayloadHeader)
	{
		logout("ERROR:  InvalidBufferException(2)\n");
		exit(0);
	}
	#endif

	return mpPayloadHeader;
}


/**
* Remember the begining of the payload inside the HPRPC command.
*/
void CHPRPCCommand::SetPayloadHeaderPtr(uint8_t* PayloadHeaderPtr)
{
	mpPayloadHeader = PayloadHeaderPtr;
}
