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

#include "StdAfx.h"

#include "CPCIeConnection.h"
#include "CDSPC8681.h"
#include "CTimePoint.h"
#include "CBufferedWriter.h"

//Headers of the Advantech DSPC8681 driver
#include "Device.h"
#include "Common.h"
#include "Ioctls.h"
#include "Ti667xDevice.h"
#include "Dspc868xCard.h"
#include "Dspc8681Card.h"

#define DMA_MEMORY_BASE             TI667X_EP_DDR3_BASE		//keep in sync with .cfg of DSP software Program.sectMap[".hprpcDMA"].loadAddress
#define BUFSIZE_HPRPC_DMA			0x4000004	//keep in sync with .cfg of DSP software Program.global.BUFSIZE_HPRPC_DMA
#define MAX_CPU_BASED_TRANSFER_SIZE 0x1000000	//Max. size of a single DMA transder (DDR3 BAR)
#define TIMEOUT_WAIT_FOR_INTERRUPT_ms	10000

extern PCHAR MapSystemErrorCode(DWORD Code);

//For nonblocking DMA transfer
#define DMA_CHAN_RD                         2
#define DMA_CHAN_WR                         3
#define DMA_PARAM_START_NUM                 0
#define TI667X_PCIE_MAX_IO_BUFFERS          12	//Buffer amount for all DSPs (will be divided by mu_TotalDSPAmount)
#define DSP_OB_REGION_SIZE                  0x400000	//Size of a single DMA buffer (which amount is defined in TI667X_PCIE_MAX_IO_BUFFERS)

//Initialize static members
CPCIeConnection::TDeviceConnectorMap CPCIeConnection::msDeviceConnectorMap;

CPCIeConnection::CPCIeConnection(uint32_t DSPNo, uint32_t TotalDSPAmount, CDSPC8681& DSPC8681)
: muDSPNo(DSPNo),
  mbConnected(false),
  m_hInterruptEvent(NULL),
  m_oDSPC8681(DSPC8681),  
  m_pBufferedWriter(NULL)
  ,m_uAllocatedDMABuffersPerDSP(TI667X_PCIE_MAX_IO_BUFFERS/TotalDSPAmount)
  ,m_uUsedDMABuffers(0)

{
	//ToDo: Necessary ? Do we need a named event ? SINCE WE HAVE AN EVENT CLASS IN THE THREAD POOL, WHY NOT USING THAT ONE ;)
	msEventDescriptor = (boost::format("CPCIeConnection_EvtDscr_%1%") % DSPNo).str();

	#ifdef _DO_ERROR_CHECKS_
	if(NULL == m_oDSPC8681.getDspc8681CardDriver())
	{
		throw NullpointerException();
	}
	#endif

	#ifdef _TRACE_DMA_
	printf("CPCIeConnector for chip #%u is on address 0x%x (should be bus #%u).\n", muDSPNo, (void*)this, muDSPNo+4);
	#endif
}

CPCIeConnection::~CPCIeConnection()
{
	if(true == mbConnected)
	{
		Disconnect();
	}
}

CBufferedWriter& CPCIeConnection::GetBufferedWriter()
{
	if(NULL == m_pBufferedWriter)
	{
		throw NullpointerException();
	}
	return *m_pBufferedWriter;
}

void CPCIeConnection::Connect()
{
	#ifdef _DO_ERROR_CHECKS_
	if(true==mbConnected)
	{
		throw PCIeException();
	}
	#endif

	m_hInterruptEvent = CreateEvent(NULL, FALSE, FALSE, msEventDescriptor.c_str());
	DWORD dwError = GetLastError();

	if(dwError != ERROR_SUCCESS)
	{
		printf("can't create PCIe event. Error: %s\n", MapSystemErrorCode(dwError));
		throw PCIeException();
	}

	if(m_hInterruptEvent == NULL)
	{
		printf("Unable to create PCIe event for interrupt. Error: %s\n", MapSystemErrorCode(dwError));
		throw PCIeException();
	}

	//Remember the bus Nr of this particular DSP (for the association in the ISR)
	UCHAR busNr = muDSPNo+4;	//The Ti667xDevice::GetDeviceObject() outputs error messages when used for more than one chip. This is an alternative to reference the instance in the IRQ
	msDeviceConnectorMap[busNr] = this;

	dwError = m_oDSPC8681.getDspc8681CardDriver()->SetInterruptHandler(muDSPNo, Isr); 

	if(dwError != ERROR_SUCCESS)
	{
		printf("can't set PCIe isr. Error: %s\n", MapSystemErrorCode(dwError));
		throw PCIeException();
	}
	
	dwError = AllocateDMAMemory();
	if(dwError != ERROR_SUCCESS)
	{
		printf("cannot allocate PCIe DMA memory. Error: %s\n",  MapSystemErrorCode(dwError));
		throw PCIeException();
	}

	//Now the DMA memory is available, allocate a buffered writer, where callers can request and send buffers to
	//(Consider of returning this as a result of the connect command ...)
	ClearBufferedWriter();

	#ifdef _TRACE_DMA_
	printf("DSP connected, ready to write Buffer data to DSP\n");
	#endif

	mbConnected = true;
}

void CPCIeConnection::Disconnect()
{
	#ifdef _DO_ERROR_CHECKS_
	if(false==mbConnected)
	{
		throw PCIeException();
	}
	#endif

	//Disallow the buffered writer to request or send buffers
    m_pBufferedWriter = NULL;

	ReleaseDMAMemory();

	mbConnected = false;
}

/*
* Receive data from the DSP. This method will block until the DSP returned data.
*/
void CPCIeConnection::Receive(uint8_t* Buffer, const uint32_t BufferLength, uint32_t& RcvLen)
{
	#ifdef _DO_ERROR_CHECKS_
	if(false==mbConnected)
	{
		throw PCIeException();
	}
	#endif

	#ifdef _TRACE_DMA_
	printf("Wait for response from DSP (worker thread) '%u.\n", muDSPNo);
	#endif

	//Wait until our IQR-Handler (above) processed the data
	DWORD dwError = Ti667xDevice::WaitForEvent(m_hInterruptEvent, TIMEOUT_WAIT_FOR_INTERRUPT_ms);

	if(dwError != ERROR_SUCCESS)
	{
		printf("failed to wait for PCIe interrupt. Error: %s\n",  MapSystemErrorCode(dwError));
		throw PCIeException();
	}

	#ifdef _TRACE_DMA_
	printf("Event received from ISR. Transfer response data from DSP to PC '%u.\n", muDSPNo);
	#endif

	//We can throw away the send buffers now and allocate fresh ones.
	ClearBufferedWriter();

	//Read DSP response
	uint32_t readLength = HPRPC_MAX_RESPONSE_LEN + sizeof(t_buflen);
	dwError = ReadFromDSPByCPU(m_ResponseBuffer, readLength);	//It would be possible to read the first word (len descriptor) first and then as much bytes as necessary. But wouldn't it be faster to read a fixed aomunt of bytes at once (as all HPRPC responses are very short in our case).
	if(dwError != ERROR_SUCCESS)
	{
		printf("failed to read the PCIe response CPU based from the DSP. Error: %s\n",  MapSystemErrorCode(dwError));
		throw PCIeException();
	}


	//Return the pointer to the rcv-buffer to the application

	//Copying could be avoided here in the future (This means to secretly allocate 4 bytes before the passed buffer (like when we're sending), furthermore to write into Buffer instead of m_ResponseBuffer in the ISR above.)
	t_buflen* bufferSizePtr = ((t_buflen*)m_ResponseBuffer);
	RcvLen = (uint32_t) *bufferSizePtr;
	#ifdef _DO_ERROR_CHECKS_
	if(BufferLength < RcvLen)
	{
		printf("Received PCIe packet too big (%d > %d).\n", RcvLen, BufferLength);
		throw PCIeException();
	}
	#endif
	memcpy(Buffer, (m_ResponseBuffer + sizeof(t_buflen)), RcvLen);
}

/**
* DMA transfer (Interface ISendableBufferProvider)
* Returns false when out of DMA kernel memory (which might be extended in future versions for sending all buffers in order to obtain free buffers again ...)
*/
bool CPCIeConnection::GetNextBuffer(uint8_t*& BufferReference, uint32_t& LengthReference)
{
	if(m_uUsedDMABuffers >= m_uAllocatedDMABuffersPerDSP)
	{
		//No more buffers allocated/available (The buffer allocation happens during the application start in AllocateDMAMemory())
		return false;
	}

	BufferReference = (uint8_t*) m_oDSPC8681.getDspc8681CardDriver()->GetDmaBufferPointer(muDSPNo, m_uUsedDMABuffers);	
	LengthReference = DSP_OB_REGION_SIZE;

	m_uUsedDMABuffers++;
	return true;
}

/**
* DMA based transfer, nonblocking (Interface ISendableBufferProvider)
*/
void CPCIeConnection::Send(uint32_t dataLength)
{
	#ifdef _DO_ERROR_CHECKS_
	if(false==mbConnected)
	{
		throw PCIeException();
	}
	#endif

	const uint32_t target_addr = DMA_MEMORY_BASE;
	const BOOL BlockingBehavior = 0;	//0=nonblocking
	const uint32_t dwError = m_oDSPC8681.getDspc8681CardDriver()->WriteDma(muDSPNo, target_addr, m_uUsedDMABuffers, dataLength, DMA_CHAN_WR, DMA_PARAM_START_NUM + m_uAllocatedDMABuffersPerDSP, BlockingBehavior);

	#ifdef _TRACE_DMA_
	printf("Finished DMA WRITE transfer DSP=%u, returncode=%u.\n", muDSPNo, dwError);
	#endif

	if(dwError != ERROR_SUCCESS)
	{
		printf("failed to write to dma buffer\n");
		throw PCIeException();
	}
	//Signalize to the DSP that the data transmission is running (The DSP will scan for the end marker in the data stream - which is the best tradeoff as it has no Main-CPU-Impact and also no wait-states.)
	SignalIRQOnDSP();
}


void CPCIeConnection::ClearBufferedWriter()
{
	m_uUsedDMABuffers = 0;	//MUST happen before the new allocation below (because the ctor of CBufferedWriter adds the length descriptor automatically in DMA-Mode, which results in a buffer allocation ...)

	if(NULL != m_pBufferedWriter)
	{
		delete m_pBufferedWriter;
	}

	m_pBufferedWriter = new CBufferedWriter(this);
}

int CPCIeConnection::AllocateDMAMemory()
{
	int iRet = m_oDSPC8681.getDspc8681CardDriver()->AllocateDmaMemory(muDSPNo, DSP_OB_REGION_SIZE, m_uAllocatedDMABuffersPerDSP);

	#ifdef _TRACE_DMA_
	printf("Allocated DMA memory for DSP %u, returncode=%u.\n", muDSPNo, iRet);
	#endif

	return iRet;
}

void CPCIeConnection::ReleaseDMAMemory()
{
	Dspc8681Card* pDev = m_oDSPC8681.getDspc8681CardDriver();
	if(NULL != pDev)
	{
		int iRet = pDev->FreeDmaMemory(muDSPNo);
		//#ifdef _TRACE_DMA_
		printf("Released DMA memory for DSP %u, returncode=%u.\n", muDSPNo, iRet);
		//#endif
		//Sleep(100);
	}
}

/**
* We don't use DMA for reading. Instead we use CPU based transfer. (Reason: We allways read only a few bytes, the lower latency of a CPU read is more benefitial here than the parallelism of DNA.)
*/
int CPCIeConnection::ReadFromDSPByCPU(uint8_t* buffer, uint32_t size)
{
	const uint32_t target_addr = DMA_MEMORY_BASE;
	int ret = m_oDSPC8681.getDspc8681CardDriver()->ReadDsp(muDSPNo, target_addr, buffer, size);
	return ret;
}

/**
* Wake up the DSP. There is data available/about to come (the DSP will start scanning for the end token @see CBufferedWriter::EnqueueEndToken()).
*/
DWORD CPCIeConnection::SignalIRQOnDSP()
{
	//Signal IRQ on DSP
	ULONG IRQIndex = 0;
	DWORD dwErr = m_oDSPC8681.getDspc8681CardDriver()->SetPciMsiIrqEnable(muDSPNo, IRQIndex);
	if(ERROR_SUCCESS != dwErr)
	{
		printf("Unable to signal data arrived IRQ on DSP %u, returncode=%u.\n", muDSPNo, dwErr);
	}
	return dwErr;
}

/**
* Instance-assigned handler of the ISR (will be called from the static ISR entry point)
*/
void CPCIeConnection::IsrHandler()
{
	DWORD dwError = ERROR_SUCCESS;

	#ifdef _TRACE_DMA_
	printf("ISR: Instance for DSP '%u entered. Wake up worker thread.\n", muDSPNo);
	#endif

	BOOL bRet=TRUE;

	bRet = SetEvent(m_hInterruptEvent);

	if(bRet == FALSE)
	{
		dwError = GetLastError();
		printf("ISR: Unable to set PCIe event. Error: %s\n", MapSystemErrorCode(dwError));
		return;
	}
}

/**
* Static ISR entry point. Will pass the call to the matching instance of CPCIeConnection.
*/
void CPCIeConnection::Isr(Ti667xDevice* pTi667xDev)
{
	#ifdef _TRACE_DMA_
	printf("ISR: Data is avilable on DSP Ptr=%x.\n", (void*)pTi667xDev);
	#endif

	UCHAR busNr = pTi667xDev->GetBusNumber();

	#ifdef _TRACE_DMA_
	printf("ISR: pTi667xDev->GetBusNumber() returns %u, this matches to CPCIeConnection Ptr=", busNr);
	printf("0x%x\n", (void*)(msDeviceConnectorMap[busNr]));
	#endif

	CPCIeConnection* pInstance = msDeviceConnectorMap[busNr];

	#ifdef _DO_ERROR_CHECKS_
	if(NULL == pInstance)
	{
		printf("ISR: ERROR IN ISR CALLBACK. UNKNOWN DEVICE POINTER 0x%X.", (void*)pTi667xDev);
		return;
	}
	#endif

	pInstance->IsrHandler();
}
