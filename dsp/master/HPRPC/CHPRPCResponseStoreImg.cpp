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

#include "CHPRPCResponseStoreImg.h"
#include "HPRPCCommon.h"

CHPRPCResponseStoreImg::CHPRPCResponseStoreImg()
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
* Used when assembling a command that has to be sent. Call this method, afterwards call CHPRPCCommand::SendToOtherPeer() to
* send a HPRPC command.
*
* As this is a response call to a HPRPC request in the parameter CallIndex the callx index of the associated request
* must be added. As returncode SUCCESS will be sent. If the caller wants to indicate an error condition instead of
* AssignValuesToBeSent() the method AssignErrorToBeSent() (public in the baseclass) has to be called by the user of
* this class.
*
* This method will allocate an internal buffer. The buffer will be freed when this obejct is destroyed.
*/
void CHPRPCResponseStoreImg::AssignValuesToBeSent(const uint16_t CallIndex, const uint32_t TimeForAddMargins, const uint32_t TimeForPyramid)
{
	//Allocate memory
	const uint32_t ciBufferLen = sizeof(HPRPC_HDR_Response_StoreImg);
	AllocateBufferWithHPRPCHeader(ciBufferLen, HPRPC_RET_SUCCESS, CallIndex);

	HPRPC_HDR_Response_StoreImg* pPayloadHeader = GetPayloadHeader();
	pPayloadHeader->CyclesForAddMargins = TimeForAddMargins;
	pPayloadHeader->CyclesForPyramid = TimeForPyramid;
}
