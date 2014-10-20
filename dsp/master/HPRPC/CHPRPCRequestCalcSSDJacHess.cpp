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

#include "CHPRPCRequestCalcSSDJacHess.h"
#include "HPRPCCommon.h"

CHPRPCRequestCalcSSDJacHess::CHPRPCRequestCalcSSDJacHess(CalcSSDMode CalculationMode)
: meCalculationMode(CalculationMode)
{
}

CHPRPCRequestCalcSSDJacHess::~CHPRPCRequestCalcSSDJacHess()
{
}

/**
* Return a pointer to the internal command specific header (exception if no header assigned)
*/
HPRPC_HDR_Request_CalcSSDJacHess* CHPRPCRequestCalcSSDJacHess::GetPayloadHeader()
{
	uint8_t* pHeader = GetPayloadHeaderPtr();

	if(NULL == pHeader)
	{
		logout("ERROR:  InvalidBufferException(4)\n");
		exit(0);
	}

	return (HPRPC_HDR_Request_CalcSSDJacHess*)pHeader;
}

/**
* Used when receiving an incoming command.
*
* Sets some pointers to the right places in the input data buffer. Use the accessor methods to obtain
* the pointers afterwards.
*
* CAREFUL: WILL NOT COPY THE BUFFER (for execution speed reasons). Hence this object
* must be deleted before the receive buffer is freed.
*/
void CHPRPCRequestCalcSSDJacHess::AssignRcvBuffer(const uint8_t* Buffer, const uint32_t BufferLen)
{
	/*
	//Assign HPRPC header
	uint32_t iPayloadLen = CHPRPCRequest::AssignRcvBuffer(Buffer, BufferLen);
	*/

	//We need to copy the header for 8-byte-struct aligning (pcie DMA buffers might not be 8-byte-aligned)	//ToDo: This was necessary in the ethernet version. Byte-Align-Copying might have become unnecessary by the PCIe DMA layout.
	const uint32_t iSizeOfAllHeaders = sizeof(HPRPC_HDR_Request) + sizeof(HPRPC_HDR_Request_CalcSSDJacHess);
	uint8_t* pHeaderCopy = CHPRPCCommand::AllocateBuffer(iSizeOfAllHeaders);
	memcpy(pHeaderCopy, Buffer, iSizeOfAllHeaders);

	//Assign HPRPC header
	//It is ok to pass the non-junct parameters pHeaderCopy and BufferLen as BufferLen is only used to
	//calculate the return value
	uint32_t iPayloadLen = CHPRPCRequest::AssignRcvBuffer(pHeaderCopy, BufferLen);

	//Check received buffer size
	if(sizeof(HPRPC_HDR_Request_CalcSSDJacHess) > iPayloadLen)
	{
		logout("ERROR:  HPRPCMalformedException()\n");
		exit(0);
	}

	//debugging
	#ifdef _TRACE_OUTPUT_
	HPRPC_HDR_Request_CalcSSDJacHess* pPayloadHeader = GetPayloadHeader();
	t_reg_real w1=pPayloadHeader->w[0];
	t_reg_real w2=pPayloadHeader->w[1];
	t_reg_real w3=pPayloadHeader->w[2];
	logout("CalcSSD_J_H Request w=[%f, %f, %f] level=%d (on %u cores)\n", (double)w1, (double)w2, (double)w3, pPayloadHeader->Level, pPayloadHeader->NumberOfCores);

	#endif
}

