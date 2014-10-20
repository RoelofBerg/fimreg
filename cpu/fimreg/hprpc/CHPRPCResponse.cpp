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

#include "HPRPCCommon.h"
#include "CHPRPCResponse.h"
#include "CHPRPCRequest.h"

/**
* ctor
* (For clarity and by purpose we will provide no overload that calls AssignRcvBuffer() !)
*/
CHPRPCResponse::CHPRPCResponse(CHPRPCConnection& HPRPCConnection)
: CHPRPCCommand(HPRPCConnection),
  mpHPRPCResponseHeader(0)
{
}

CHPRPCResponse::~CHPRPCResponse(void)
{
}

/**
* If an error code was returned this method returns the error string.
* The user of the HPRPC classes will usually notice about an error when calling the AssignRcvBuffer()
* method of a response object specialisation and evaluating its returncode. The next step the user
* of that class should do is call this method GetErrorDescription() to obtain further information.
*/
void CHPRPCResponse::GetErrorDescription(string& ErrorDescription)
{
	uint32_t iPayloadLen = mpHPRPCResponseHeader->DataLen - sizeof(HPRPC_HDR_Response);
	ErrorDescription = std::string(reinterpret_cast<char *>(mpHPRPCResponseHeader), iPayloadLen);
}

/**
* Return the call index of the response call
*/
uint16_t CHPRPCResponse::GetCallIndex()
{
	if(NULL == mpHPRPCResponseHeader)
	{
		throw InvalidBufferException();
	}

	//A set MSB in the call index marks the HPRPC package to be a response (and no request).
	//This MSB is not interesting for the caller, so it will be stripped off here.
	const uint16_t iCallIndexWithMSBUnset = mpHPRPCResponseHeader->CallIdx & 0x7fff;

	return iCallIndexWithMSBUnset;
}

/**
* Called from baseclass (baseclass calls superclass by namespace).
* Assign the passed buffer to this class. No memory will be copied for
* speed enhancement. Hence the caller MUST make sure that the passed
* Buffer is as long valid as this class exists.
*
* The return code will be passed back using the out parameter Out_ReturnCode. If the return code
* does not match HPRPC_RET_SUCCESS the user should not process the command ans should use the
* GetErrorDescription() method to retreive further information.
*
* Size of payload within buffer will be returned to the base class (matching to buffer returned by GetPayloadHeader())
*/
uint32_t  CHPRPCResponse::AssignRcvBuffer(const uint8_t* Buffer, const uint32_t BufferLen, uint16_t& Out_ReturnCode)
{
	//Check received buffer size
	if(sizeof(HPRPC_HDR_Response)>BufferLen)
	{
		throw HPRPCMalformedException();
	}

	//Parse and process
	mpHPRPCResponseHeader = (HPRPC_HDR_Response*)Buffer;
	SetPayloadHeaderPtr((uint8_t*)mpHPRPCResponseHeader + sizeof(HPRPC_HDR_Response));

	//Pass back return code
	Out_ReturnCode = mpHPRPCResponseHeader->Returncode;

	//return buffer size
	return (BufferLen - sizeof(HPRPC_HDR_Response));
}


/**
* Returns true if the returncode of the response is valid and if the
* CallIndex of the matching request is equal to the returned CallIndex.
*
* Outputs the error message text if an error message was received from
* the remote peer.
*/
bool CHPRPCResponse::IsResponseValid(CHPRPCRequest& Request, const uint32_t DSPIdx, const string& CommandLogString)
{
	//Sanity
	if(NULL == mpHPRPCResponseHeader)
	{
		throw InvalidBufferException();
	}

	//Does the call index of the response match to our expected call index ?
	//Because TCP guarantees package order we can just throw an error if our expectation won't be matched.
	if(Request.GetCallIndex() != GetCallIndex())
	{
		printf("[DSP_%u] HPRPC received unexpected call index in HPRPC response to command '%s'. Expected %u, received %u.\n", DSPIdx, CommandLogString.c_str(), Request.GetCallIndex(), GetCallIndex());
		return false;
	}

	//Is the returncode a success returncode ?
	if(mpHPRPCResponseHeader->Returncode != HPRPC_RET_SUCCESS)
	{
		string sErrorDscr;
		GetErrorDescription(sErrorDscr);
		printf("[DSP_%u] HPRPC received error message in response to to command '%s'. Message text: '%s'.\n", DSPIdx, CommandLogString.c_str(), sErrorDscr.c_str());
		return false;
	}

	return true;
}
