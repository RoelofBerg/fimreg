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

#include "CHPRPCRequestCalcSSDJacHess.h"
#include "HPRPCCommon.h"
#include "CBufferedWriter.h"

CHPRPCRequestCalcSSDJacHess::CHPRPCRequestCalcSSDJacHess(CHPRPCConnection& HPRPCConnection, CalcSSDMode CalculationMode)
: CHPRPCRequest(HPRPCConnection),
  meCalculationMode(CalculationMode)
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
		throw InvalidBufferException();
	}

	return (HPRPC_HDR_Request_CalcSSDJacHess*)pHeader;
}

/**
* Used when assembling a command that has to be sent. Call this method, afterwards call CHPRPCCommand::SendToOtherPeer() to
* send a HPRPC remote procedure call.
*
* Returns the auto-incremented index which is a reference between request HPRPC command and response HPRPC command.
*
* This method will allocate an internal buffer. The buffer will be freed when this obejct is destroyed.
*/
void CHPRPCRequestCalcSSDJacHess::AssignValuesToBeSent(const t_reg_real w[3], const uint16_t NumberOfCores, const uint16_t Level)
{
	//Push the general HPRPC header into the DMA memory buffer
	const uint32_t ciBufferLen = sizeof(HPRPC_HDR_Request_CalcSSDJacHess);
	const uint16_t iOpCode = (meCalculationMode == CALC_SSD_JAC_HESS) ? HPRPC_OP_CALC_SSD_JAC_HESS : HPRPC_OP_CALC_SSD;
	AssignRequestHeader(ciBufferLen, iOpCode);

	//Prepare header of this particular HPRPC command
	HPRPC_HDR_Request_CalcSSDJacHess oPayloadHeader;
	oPayloadHeader.NumberOfCores = NumberOfCores;
	oPayloadHeader.Level = Level;
	memcpy(&(oPayloadHeader.w), w, 3*sizeof(t_reg_real));	//Note: ti and microsoft behave differently if var (instead of type) is passed to sizeof. So we use the type for clarity.

	//Push the command's header into the DMA memory buffer
	CBufferedWriter& oBufferedWriter = GetBufferedWriter();
	oBufferedWriter.Enqueue((uint8_t*)(&oPayloadHeader), ciBufferLen);

	//debugging
	#ifdef _TRACE_OUTPUT_
	t_reg_real w1=oPayloadHeader.w[0];
	t_reg_real w2=oPayloadHeader.w[1];
	t_reg_real w3=oPayloadHeader.w[2];
	const char* sCalcMode = (CALC_SSD_JAC_HESS==meCalculationMode) ? "CalcSSD_J_H" : "CalcSSD";
	printf("%s Request w=[%f, %f, %f] level=%u (on %u cores)\n", sCalcMode, w1, w2, w3, Level, NumberOfCores);
	#endif
}