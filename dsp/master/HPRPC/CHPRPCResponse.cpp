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

#include "CHPRPCResponse.h"

/**
* ctor
*/
CHPRPCResponse::CHPRPCResponse(void)
: mpHPRPCResponseHeader(0)
{
}

CHPRPCResponse::~CHPRPCResponse(void)
{
}

/**
* The base classes have the method AssignValuesToBeSend() (with class specific parameter list). If the caller wants to report
* an error message instead of a successful response (with data) the caller can use this method instead. As it is equal for all
* derived classes it is located in the base class. It is not intended to be called by the derived classes, it is intended to
* be called by the user of the derived class.
*/
void CHPRPCResponse::AssignErrorToBeSent(const uint16_t CallIndex, const uint16_t ErrorCode, const string& ErrorMessage)
{
	//Allocate memory
	const uint32_t ciBufferLen = sizeof(HPRPC_HDR_Response) + ErrorMessage.size();
	AllocateBufferWithHPRPCHeader(ciBufferLen, ErrorCode, CallIndex);

	//Copy error message into the payload data buffer (without null termination)
	memcpy(GetPayloadHeaderPtr(), ErrorMessage.c_str(), ErrorMessage.size());
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
* Allocate memory for assembling an HPRPC package. The size of the data WITHOUT the CmdHeader
* has to be passed (only the size of the data to be transmitted as payload). The size of the
* header will automatically be added.
*
* Begin of payload within buffer can be accessed by  GetPayloadHeader().
*/
void CHPRPCResponse::AllocateBufferWithHPRPCHeader(const uint32_t DataLenWithoutCmdHeader, const uint16_t Returncode, const uint16_t CallIndex)
{
	//Allocate Buffer for HPRPC command
	uint32_t BufferLen = DataLenWithoutCmdHeader + sizeof(HPRPC_HDR_Response);
	uint8_t* Buffer = CHPRPCCommand::AllocateBuffer(BufferLen);
	mpHPRPCResponseHeader = (HPRPC_HDR_Response*)Buffer;
	SetPayloadHeaderPtr(Buffer + sizeof(HPRPC_HDR_Response));

	//Write header data into the buffer
	AssignResponseHeader(Returncode, CallIndex, BufferLen);
}

/**
* Assign header data for the HPRPC request
*
* Used when assembling a response to be sent (after AllocateBuffer()). CallIndex must be set to the
* CallIndex of the call this response belongs to (is the answer to).
*/
void CHPRPCResponse::AssignResponseHeader(const uint16_t Returncode, const uint16_t CallIndex, const uint32_t DataLen)
{
	//A set MSB in the call index marks the HPRPC package to be a response (and no request).
	const uint16_t iCallIndexWithMSBSet = CallIndex | 0x8000;

	mpHPRPCResponseHeader->CallIdx = iCallIndexWithMSBSet;
	mpHPRPCResponseHeader->Returncode = Returncode;
	mpHPRPCResponseHeader->DataLen = DataLen;
}
