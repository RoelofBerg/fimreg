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

#pragma once

#include "dspreg_common.h"
#include "CMatlabArray.h"
#include "CHPRPCRequestStoreImg.h"

class CHPRPCServer;

/**
* Use the compileswitch SIMULATE_TARGET to simulate a DSP target connection on the local PC.
* Instead of sockets streams will be used to pass the data ...
*
* Create several instances to simulate several DSP processors at once
*/
class CTargetConnector
{
public:
	CTargetConnector();
	virtual ~CTargetConnector();

	void Reinitialize(CHPRPCServer* HPRCPServer);
	void ProcessIncomingCommand(const uint8_t* Buffer, const uint32_t BufferLen, bool NotifySenderWhenTransferFinished);

private:
	void Reinitialize();

	//OpCode Commands
	void Process_HPRPC_StoreImage_Request(const uint8_t* Buffer, const uint32_t BufferLen);

	void Process_HPRPC_CalcSSDJacHess_Request(const uint8_t* Buffer, const uint32_t BufferLen);
	t_reg_real CalcSSDJacHess(const uint32_t level, const t_reg_real w[3], t_reg_real JD[3], t_reg_real JD2[9], const uint32_T number_of_cores);

	void Process_HPRPC_CalcSSD_Request(const uint8_t* Buffer, const uint32_t BufferLen);
	t_reg_real CalcSSD(const uint32_t level, const t_reg_real w[3], const uint32_T number_of_cores);

	void Process_HPRPC_CalcFinished_Request(const uint8_t* Buffer, const uint32_t BufferLen);

	//Helpers
	void ExtractLevel(const uint32_t level, uint32_t BoundBox[4], uint32_t MarginAddon[3], t_reg_real DSPRange[4], uint32_t &TOffset, uint32_t &ROffset);

	/** StoreImageRequest will be processed using several incoming calls. (It will contain finally the RLE decoded picture data as a Matlab compatible array.)*/
	CHPRPCRequestStoreImg mStoreImgRequest;
	bool mbReceivingAnImageRequest;

	/** Saves performance, see description of the Reinitialize() method. @see Reinitialize()*/
	bool mbInitNecessary;

	//Backwards reference for sending response packets
	CHPRPCServer* m_pHPRCPServer;
};
