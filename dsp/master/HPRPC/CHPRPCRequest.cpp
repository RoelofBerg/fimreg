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

#include "CHPRPCRequest.h"

/**
* Ctor
* (For clarity and by purpose we will provide no overload that calls AssignRcvBuffer() !)
*/
CHPRPCRequest::CHPRPCRequest(void)
: mpHPRPCRequestHeader(0)
{
}

CHPRPCRequest::~CHPRPCRequest(void)
{
}

/**
* Return the call index of the current HPRPC header
*/
uint16_t CHPRPCRequest::GetCallIndex()
{
	if(NULL == mpHPRPCRequestHeader)
	{
		logout("ERROR:  InvalidBufferException(3)\n");
		exit(0);
	}

	return mpHPRPCRequestHeader->CallIdx;
}

/**
* Called from baseclass (baseclass calls superclass by namespace).
* Assign the passed buffer to this class. No memory will be copied for
* speed enhancement. Hence the caller MUST make sure that the passed
* Buffer is as long valid as this class exists.
*
* Size of payload within buffer will be returned to the base class (matching to buffer returned by GetPayloadHeader())
*/
uint32_t CHPRPCRequest::AssignRcvBuffer(const uint8_t* Buffer, const uint32_t BufferLen)
{
	//Check received buffer size
	if(sizeof(HPRPC_HDR_Request)>BufferLen)
	{
		logout("ERROR:  HPRPCMalformedException()\n");
		exit(0);
	}

	//Parse and process
	mpHPRPCRequestHeader = (HPRPC_HDR_Request*)Buffer;
	SetPayloadHeaderPtr((uint8_t*)mpHPRPCRequestHeader + sizeof(HPRPC_HDR_Request));

	//return buffer size
	return (BufferLen - sizeof(HPRPC_HDR_Request));
}

/**
* Increments the global call index. Wraps around at 15 bits.
* Bit 16 is reserved for distinguishing between request and
* response.
*
* CAREFUL: The current implementation is not threadsave.
* (As only one send thread will be used threadsafety isn't
* necessary. However if in the future several threads will
* send commands some kind of synchronization will be necessary.)
*/
uint16_t CHPRPCRequest::GetNextCallIndex()
{
	//Static variable containing call index.
	//(Encapsulated within this function because threadsynchronization
	//code might be added in the future.)
	//
	//Note: The start-value 0xffff will cause the 0 to be used as
	//first value after the very first incrementation.
	static uint16_t giCallIndex = 0xffff;

	giCallIndex++;
	if(giCallIndex > 0x7fff)
	{
		//Wrap around before setting MSB to 1, avoid reserved value 0
		giCallIndex=1;
	}
	return giCallIndex;
}
