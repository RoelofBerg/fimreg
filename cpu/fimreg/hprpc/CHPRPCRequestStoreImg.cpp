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

#include "CHPRPCRequestStoreImg.h"
#include "CBufferedWriter.h"
#include "HPRPCCommon.h"
#include "generatePyramidPC.h"

#include "TimingUtils.h"

//Command line parameter
extern uint32_t guClipDarkNoise;

CHPRPCRequestStoreImg::CHPRPCRequestStoreImg(CHPRPCConnection& HPRPCConnection)
: 	CHPRPCRequest(HPRPCConnection)
{
}

CHPRPCRequestStoreImg::~CHPRPCRequestStoreImg(void)
{
}

/**
* Return a pointer to the internal command specific header (exception if no header assigned)
*/
HPRPC_HDR_Request_StoreImg* CHPRPCRequestStoreImg::GetPayloadHeader()
{
	uint8_t* pHeader = GetPayloadHeaderPtr();

	#ifdef _DO_ERROR_CHECKS_
	if(NULL == pHeader)
	{
		throw InvalidBufferException();
	}
	#endif

	return (HPRPC_HDR_Request_StoreImg*)pHeader;
}

/**
* Used when assembling a command that has to be sent. Call this method, afterwards call CHPRPCCommand::SendToOtherPeer() to
* send a HPRPC remote procedur call command.
*
* The function will RLE compress the image data before sending.
*
* Returns the auto-incremented index which is a reference between request HPRPC command and response HPRPC command.
*
* This method will allocate an internal buffer. The buffer will be freed when this obejct is destroyed.
*/
void CHPRPCRequestStoreImg::AssignValuesToBeSent(const uint32_t BoundBox[4], const uint32_t MarginAddon[3], const t_reg_real DSPResponsibilityBox[4], const uint32_t TSinglePartLen, const uint32_t RSinglePartLen, const uint32_t d, const uint16_t levels, const uint16_t NumberOfCores)
{
	//Push the general HPRPC header into the DMA memory buffer
	const uint32_t ciBufferLen = sizeof(HPRPC_HDR_Request_StoreImg);
	AssignRequestHeader(ciBufferLen, HPRPC_OP_STORE_IMG);

	//Prepare header of this particular HPRPC command
	HPRPC_HDR_Request_StoreImg oPayloadHeader;
	memcpy(&(oPayloadHeader.BoundBox), BoundBox, 4 * sizeof(uint32_t)); 	//Note: ti and microsoft behave differently if var (instead of type) is passed to sizeof. So we use the type for clarity.
	memcpy(&(oPayloadHeader.MarginAddon), MarginAddon, 3 * sizeof(uint32_t)); 	//Note: ti and microsoft behave differently if var (instead of type) is passed to sizeof. So we use the type for clarity.
	memcpy(&(oPayloadHeader.DSPResponsibilityBox), DSPResponsibilityBox, 4 * sizeof(t_reg_real)); 	//Note: ti and microsoft behave differently if var (instead of type) is passed to sizeof. So we use the type for clarity.
	oPayloadHeader.TSinglePartLen = TSinglePartLen;
	oPayloadHeader.RSinglePartLen = RSinglePartLen;
	oPayloadHeader.d = d;
	oPayloadHeader.levels = levels;
	oPayloadHeader.NumberOfCores = NumberOfCores;

	//Push the command's header into the DMA memory buffer
	CBufferedWriter& oBufferedWriter = GetBufferedWriter();
	oBufferedWriter.Enqueue((uint8_t*)(&oPayloadHeader), ciBufferLen);

	//debugging
	#ifdef _TRACE_OUTPUT_
	uint32_t b1=pPayloadHeader->BoundBox[0];
	uint32_t b2=pPayloadHeader->BoundBox[1];
	uint32_t b3=pPayloadHeader->BoundBox[2];
	uint32_t b4=pPayloadHeader->BoundBox[3];
	t_reg_real d1=pPayloadHeader->DSPResponsibilityBox[0];
	t_reg_real d2=pPayloadHeader->DSPResponsibilityBox[1];
	t_reg_real d3=pPayloadHeader->DSPResponsibilityBox[2];
	t_reg_real d4=pPayloadHeader->DSPResponsibilityBox[3];
	uint32_t dim=pPayloadHeader->d;
	uint32_t lev=pPayloadHeader->levels;
	printf("StoreImg Request D=%u L=%u BB=[%u, %u, %u, %u] DB=[%f, %f, %f, %f]\n",
			dim, lev, b1, b2, b3, b4, d1, d2, d3, d4);
	#endif
}

