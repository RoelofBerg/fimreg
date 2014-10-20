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

#include "fimreg_common.h"
#include "ISendableBufferProvider.h"

//forward declarations
class Ti667xDevice;
class CDSPC8681;
class CBufferedWriter;

#define HPRPC_MAX_RESPONSE_LEN 64	//In bytes. (ToDo: Caclulate from header sizes (sizeof(t_buflen) + sizeof(HPRPC_HDR_Response) + sizeof(HPRPC_HDR_Response_CalcSSDJacHess)))
enum
{
    chip = 0, core = 1, entry = 2
};

/**
* Connection to one DSP chip. (The card that is carrying 4 DSPs is represented by CDSPC8681).
*
* Note: The Advantech API looks like one could use one Ti667xDevice per CPCIeConnection.
* This does not work. A Ti667xDevice is only compatible to one connection to one DSP per process (at least in the current driver version).
* Therefore we use an instance of Dspc8681Card here. (And have some hassle in the Advantech-ISR which has no userdefine void*, but a
* Ti667xDevice* instead that doesn't know it's own chip-no, but it's pTi667xDev->GetBusNumber() ...)
*/
class CPCIeConnection : public ISendableBufferProvider
{
public:
	CPCIeConnection(uint32_t DSPNo, uint32_t TotalDSPAmount, CDSPC8681& DSPC8681);
	virtual ~CPCIeConnection();

	void Connect();
	void Disconnect();

	//CPU based transfer
	void Receive(uint8_t* Buffer, const uint32_t BufferLength, uint32_t& RcvLen);

	//DMA transfer (Interface ISendableBufferProvider)
	CBufferedWriter& GetBufferedWriter();
	bool GetNextBuffer(uint8_t*& BufferReference, uint32_t& LengthReference);
	void Send(uint32_t dataLength);
	void ClearBufferedWriter();

private:
	typedef std::map<UCHAR, CPCIeConnection*> TDeviceConnectorMap;

	string msEventDescriptor;
	bool mbConnected;
	uint32_t muDSPNo;
	static TDeviceConnectorMap msDeviceConnectorMap;
	uint32_t m_uUsedDMABuffers;	//ToDo: It'd be a good idea to extract the buffer stuff in it's own class ... See also comment in CBufferedWriter for resolving a circular dependency.
	uint32_t m_uAllocatedDMABuffersPerDSP;

	HANDLE m_hInterruptEvent;
	BYTE m_ResponseBuffer[HPRPC_MAX_RESPONSE_LEN + sizeof(t_buflen)];

	CDSPC8681& m_oDSPC8681;
	CBufferedWriter* m_pBufferedWriter;

	int AllocateDMAMemory();
	void ReleaseDMAMemory();
	int ReadFromDSPByCPU(uint8_t* buffer, uint32_t size);
	

	DWORD SignalIRQOnDSP();

	//Interrupt callback
	static void Isr(Ti667xDevice* pTi667xDev);
	void IsrHandler();
};
