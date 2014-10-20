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

/*
 * CHPRPCServer.cpp
 *
 *  Created on: 06.07.2012
 *      Author: Roelof
 */

#include "stdafx.h"	//Precompiled headerfile, MUST be first entry in .cpp file
#include "CHPRPCServer.h"

#include "CTargetConnector.h"
#include "CHPRPCRequest.h"						//for knowing about the packet size
#include "CHPRPCRequestCalcSSDJacHess.h"		//for knowing about the packet size
#include "cache_handling.h"

#include <xdc/cfg/global.h>	//constants from .cfg file like BUFSIZE_HPRPC_DMA

extern "C" void hprpc_send_irq_to_host();

CHPRPCServer g_HPRPCServer;	//The new operator doesn't work, hence I initialize here very early instead of providing an extern "C" init function (which would create the instance by using 'new').

typedef uint32_t t_buflen;

/**
 * Called from the ISRs when PCIe data arrives
 */
extern "C" void ProcessIncomingDataFrame(uint8_t* InBuffer, uint32_t InBufferLength, uint8_t* OutBuffer, uint32_t MaxOutBufferLength)
{

	g_HPRPCServer.ProcessIncomingDataFrame(InBuffer, InBufferLength, OutBuffer, MaxOutBufferLength);
}

//Test memory destination
//#define BufLen2 (1024*1024*300)	//300 MB
//static char pBuf2[BufLen2];

/**
 * ctor
 */
CHPRPCServer::CHPRPCServer()
: mOutBuffer(NULL),
  mMaxOutBufferLength(0)
{
	mTargetConnector.Reinitialize(this);
}

/**
 * dtor
 */
CHPRPCServer::~CHPRPCServer()
{
	//Note: dtor unused in current design
}

/**
 * Incoming data comes in packets (due to the Ethernet history of this application and because of the BAR length limitation in PCIe we kept this structure).
 *
 * The incoming data buffer is passed here as well as the output data buffer the response shall be written to. Currently this is the same buffer but this
 * kind of flexibility might be necessary in the future for performance optimization (e.g. when several buffers will be used or when response data will be
 * sent before the request has been fully processed etc.)
 */
void CHPRPCServer::ProcessIncomingDataFrame(const uint8_t* InBuffer, const uint32_t InBufferLength, uint8_t* OutBuffer, const uint32_t MaxOutBufferLength)
{
	/*
	#ifdef _DO_ERROR_CHECKS_
	if(mTargetConnector.IsUninitialized())
	{
		logout("ERROR: Accessing CTargetConnector() before it has been initialized.");
	}
	#endif
	*/

	#ifdef _TRACE_PCIE_TRAFFIC_DUMP_
	logout("InBufferPtr=0x%X, InBufferLength=0x%X\n", (uint32_t)(InBuffer), InBufferLength);
	DumpMemory("InBuffer", InBuffer, InBufferLength);
	#endif

	// Invalidate cache (enough space for most all HPRPC commands expept bulk image data)
	const uint32_t cUSUAL_PAYLOAD_LENGTH = sizeof(HPRPC_HDR_Request_CalcSSDJacHess);
	const uint32_t cINVAL_FIRST_LENGTH = sizeof(HPRPC_HDR_Request) + sizeof(t_buflen) + cUSUAL_PAYLOAD_LENGTH;	//Most likely memory to be cache invalidated (only bulk image data sends will need more space)
	//logout("Cache_inv(%X, %u);\n", InBuffer, cINVAL_FIRST_LENGTH);
	Cache_inv((xdc_Ptr*) InBuffer, cINVAL_FIRST_LENGTH, Cache_Type_ALL, CACHE_BLOCKING);

	//First DWORD of buffer defines the packet length on the PCIe transport layer. (ToDo: This is somewhat redundant to the HPRPC protocol, refactor ... / Seems to be part of the Advantech CPU-Transport, which we implemented first. We added it to the DMA-code on the sender, so CPU-Mode and DMA-Mode behave equally.)
	//Find out locations(pointers) where the data size and the payload are located
	t_buflen* bufferSizePtr = ((t_buflen*)InBuffer);
	const uint8_t* bufferPayload = InBuffer + sizeof(t_buflen);

	//Find out the payload length, and who is responsible for detecting the end of the transfer (read more in the comment for CTargetConnector::ProcessIncomingCommand().)
	uint32_t inHPRPCContentLen = *bufferSizePtr;		//The latter did work for PCIE, did not for DMA. Strange ...
	const bool bNotifySenderWhenTransferFinished = (0 != (inHPRPCContentLen & 0x80000000));
	//Mask out MSB
	inHPRPCContentLen &= 0x7FFFFFFF;


	const uint32_t uTotalDataLen = inHPRPCContentLen + sizeof(t_buflen);

	#ifdef _TRACE_PCIE_TRAFFIC_DUMP_
	logout("bufferSizePtr=0x%x, inHPRPCContentLen=0x%x\n", (uint32_t)(bufferSizePtr), inHPRPCContentLen);
	#endif


	#ifdef _TRACE_HPRPC_VERBOSE_
	logout("RCV PCIe REQ ~~~~~~~~~~~~~~ (%u bytes, finished-detection by %s)\n", uTotalDataLen, bNotifySenderWhenTransferFinished ? "DSP" : "CPU");
	#endif

	#ifdef _DO_ERROR_CHECKS_
	if(uTotalDataLen > InBufferLength)
	{
		logout("ERROR: Request buffer too big.\n");
	}
	#endif

	// Invalidate more cache if necessary (enough space for most all HPRPC commands expept bulk image data) (Only when the transfer already ended, which is the case when bNotifySenderWhenTransferFinished==false)
	if((false == bNotifySenderWhenTransferFinished) &&  (uTotalDataLen > cINVAL_FIRST_LENGTH))
	{
		//logout("Cache_inv2(%X, %u);\n",  InBuffer + cINVAL_FIRST_LENGTH, uTotalDataLen - cINVAL_FIRST_LENGTH);
		Cache_inv((xdc_Ptr*) InBuffer + cINVAL_FIRST_LENGTH, uTotalDataLen - cINVAL_FIRST_LENGTH, Cache_Type_ALL, CACHE_BLOCKING);
	}

	mOutBuffer = OutBuffer;
	mMaxOutBufferLength = MaxOutBufferLength;
	mTargetConnector.ProcessIncomingCommand(bufferPayload, inHPRPCContentLen, bNotifySenderWhenTransferFinished);
}

/**
* Respond to the PC over PCIe. BufferLen must be smaller than the buffer length passed to mTargetConnector.ProcessIncomingCommand().
* The response data will be written to the buffer passed to  mTargetConnector.ProcessIncomingCommand().
*
* Note: Currently the master_main() task should call SendDataFromTargetToPC() only once per request. It is allowed to do this before
* the processing of the request has been finished. The host will continue then and will shortly send a new request. this should be
* only one request. master_main() will enter then in ProcessIncomingDataFrame() again after the last request (which was still in progress
* while the original call to this method came) will be finished. This way host and DSP can act with some kind of parallel processing, e.g.
* during pyramid generation.
*/
void CHPRPCServer::SendDataFromTargetToPC(const uint8_t* Buffer, const uint32_t BufferLen)
{
	#ifdef _DO_ERROR_CHECKS_
	  if(BufferLen > mMaxOutBufferLength)
	  {
		  logout("ERROR: Response buffer too big.");
	  }
	#endif

	// Dump answer to be passed back (request will be dumped in CTargetConnector when the HPRPC-Header has been parsed.)
	#ifdef _TRACE_HPRPC_VERBOSE_
	DumpMemory("HPRPC_Resp", Buffer, BufferLen);
	#endif

	//Write length of ouput data to the response packet
	t_buflen* bufferSizePtr = ((t_buflen*)mOutBuffer);
	*bufferSizePtr = BufferLen;

	//Copy data (todo: avoid copy operations ...)
	memcpy((void*)(mOutBuffer+sizeof(t_buflen)), (void*)Buffer, BufferLen);
	const uint32_t uOutBufferLength = BufferLen+sizeof(t_buflen);

	/* Write val into ptr, writeback cache, and wait for result to land. */
	Cache_wb((xdc_Ptr*) mOutBuffer, uOutBufferLength, Cache_Type_ALL, CACHE_BLOCKING);

	#ifdef _TRACE_HPRPC_VERBOSE_
	logout("SND PCIe RESP ~~~~~~~~~~~~~~ (%u bytes)\n", uOutBufferLength);
	#endif

	/* generate interrupt to host */
	hprpc_send_irq_to_host();
}

