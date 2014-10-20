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

#include "CHPRPCResponseStoreImg.h"
#include "HPRPCCommon.h"

CHPRPCResponseStoreImg::CHPRPCResponseStoreImg(CHPRPCConnection& HPRPCConnection)
: CHPRPCResponse(HPRPCConnection)
{
}

CHPRPCResponseStoreImg::~CHPRPCResponseStoreImg(void)
{
}

/**
* Return a pointer to the internal command specific header (exception if no header assigned)
*/
HPRPC_HDR_Response_StoreImg* CHPRPCResponseStoreImg::GetPayloadHeader()
{
	uint8_t* pHeader = GetPayloadHeaderPtr();

	if(NULL == pHeader)
	{
		logout("ERROR:  InvalidBufferException(8)\n");
		exit(0);
	}

	return (HPRPC_HDR_Response_StoreImg*)pHeader;
}

/**
* Used when receiving an incoming command.
*
* Sets some pointers to the right places in the input data buffer. Returns the HPRPC returncode. If not
* HPRPC_RET_SUCCESS is returned the error string can (and should) be optained using the GetErrorDescription()
* method from the base class.
*
* CAREFUL: WILL NOT COPY THE BUFFER (for execution speed reasons). Hence this object
* must be deleted before the receive buffer is freed.
*/
uint16_t CHPRPCResponseStoreImg::AssignRcvBuffer(const uint8_t* Buffer, const uint32_t BufferLen)
{
	//Assign HPRPC header
	uint16_t iReturnCode=HPRPC_RET_ERR_INVALID;	//AssignRcvBuffer wil set this variable to the received HPRPC return code
	uint32_t iPayloadLen = CHPRPCResponse::AssignRcvBuffer(Buffer, BufferLen, iReturnCode);

	//Check received buffer size (only if returncode was success)
	if(iReturnCode == HPRPC_RET_SUCCESS && iPayloadLen < sizeof(HPRPC_HDR_Response_StoreImg))
	{
		throw HPRPCMalformedException();
	}

	return iReturnCode;
}

