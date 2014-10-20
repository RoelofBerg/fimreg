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

/**
* CTargetConnector.cpp: Defines the entry point for the console application.
*/

//precompiled header (MUST BE THE FIRST ENTRY IN EVERY CPP FILE)
#include "stdafx.h"

#include "CTargetConnector.h"
#include "dspreg_common.h"
#include "mcip_process.h"	//Entry points for IPC distributed functions (e.g. ssd(), jacobian())

#include "HPRPCCommon.h"
#include "CHPRPCRequestStoreImg.h"
#include "CHPRPCResponseStoreImg.h"
#include "CHPRPCRequestCalcSSDJacHess.h"
#include "CHPRPCResponseCalcSSDJacHess.h"
#include "CHPRPCResponseCalcSSD.h"

#include "cache_handling.h"

CTargetConnector::CTargetConnector()
: mbReceivingAnImageRequest(false),
  m_pHPRCPServer(0),
  mbInitNecessary(true)
{
	Reinitialize();
}

CTargetConnector::~CTargetConnector()
{
	//Note: Dtor unused in current design

	//Discard (only virtually allocated - in the background it is a static buffer) memory
	//Zero out image memory to have prepared bounding boxes for the first/next calculation
	mStoreImgRequest.Reinitialize();
}

/**
 * Initialize variables, zero memory that will be used for padding. (Will zero it only after boot time
 * and after the whole calculation finished).
 *
 * We will not allocate memory on the target system (e.g. to avoid heap fragmentation as the system might run for years and
 * no MMU is present). Hence the operations of discarding an old and creating a new instance can be executed here. The result
 * is the same as if the caller would drop an old and create a new instance.
 */
void CTargetConnector::Reinitialize(CHPRPCServer* HPRCPServer)
{
	#ifdef _TRACE_SOCK_
	logout("Reinitializing Target Connector.\n");
	#endif

	if(mbInitNecessary)
		Reinitialize();
	m_pHPRCPServer=HPRCPServer;
}

/**
 * Initialize variables, zero memory that will be used for padding. (Will zero it only after boot time
 * and after the whole calculation finished.
 *
 * We will not allocate memory on the target system (e.g. to avoid heap fragmentation as the system might run for years and
 * no MMU is present). Hence the operations of discarding an old and creating a new instance can be executed here. The result
 * is the same as if the caller would drop an old and create a new instance.
 */
void CTargetConnector::Reinitialize()
{
	mbInitNecessary = false;

	//Discard (only virtually allocated - in the background it is a static buffer) memory
	//Zero out image memory to have prepared bounding boxes for the first/next calculation
	mStoreImgRequest.Reinitialize();
}

/**
 * Process data coming from the remote PC
 * Passes receive buffer, payload length (without the initial 4 byte PCIe layer header)
 *
 * Flag NotifySenderWhenTransferFinished:
 * On of both parties (sender/reveiver) HAS to do busy waiting. This flag determines who does this (true=dsp, false=pc).
 *
 * If set to true, sending the data takes place in parallel (by DMA) to the execution of this command and we are responsible to detect when the transfer has been finished.
 * Then we have to notify the client (by an IRQ) about having received all data.
 *
 * If set to false, the client detected the transfer-finished-event and called us (by DMA) when all data already arrived at the DSP. Then we can just process the data.
 */
void CTargetConnector::ProcessIncomingCommand(const uint8_t* Buffer, const uint32_t BufferLen, bool NotifySenderWhenTransferFinished)
{
	//Ignore empty commands (can happen at startup when the memset(0)ized dummy_buffer will be used in a bogus irq-trigger ?)
	if(0 == BufferLen)
	{
		#ifdef _TRACE_HPRPC_VERBOSE_
		logout("Ignoring HPRPC command with length 0 (length incl. HPRPC header).\n");
		#endif
		return;
	}

	//Wait by busy waiting until the DMA transmission of the sender finished (explanation see method comment)
	uint32_t BufferLen_wo_EndToken = BufferLen;
	if(true == NotifySenderWhenTransferFinished)
	{
		const uint32_t ETL=4;	//End Token Length
		BufferLen_wo_EndToken -= ETL;	//We expect an end-token (only when NotifySenderWhenTransferFinished is set)

//Cache_inv((xdc_Ptr*) Buffer, BufferLen, Cache_Type_ALL, CACHE_BLOCKING);	//+-ETL: The first bytes were already made sure to have been received
//logout("Buffer=0x%X, Len=0x%X\n", Buffer, BufferLen);
//logout("EndToken at adr=0x%X is 0x%X.\n",  &Buffer[BufferLen_wo_EndToken], (uint32_t) *((uint8_t*)(&Buffer[BufferLen_wo_EndToken])));
//logout("EndToken at adr=0x%X is 0x%X.\n",  &Buffer[BufferLen_wo_EndToken+1], (uint32_t) *((uint8_t*)(&Buffer[BufferLen_wo_EndToken+1])));
//logout("EndToken at adr=0x%X is 0x%X.\n",  &Buffer[BufferLen_wo_EndToken+2], (uint32_t) *((uint8_t*)(&Buffer[BufferLen_wo_EndToken+2])));
//logout("EndToken at adr=0x%X is 0x%X.\n",  &Buffer[BufferLen_wo_EndToken+3], (uint32_t) *((uint8_t*)(&Buffer[BufferLen_wo_EndToken+3])));
//logout("PCIe header at adr=0x%X is 0x%X.\n",  &Buffer[-4], *((uint32_t*)(&Buffer[-4])));
//logout("Data at start adr=0x%X is 0x%X.\n",  &Buffer[0], *((uint32_t*)(&Buffer[0])));
//logout("Data at adr=0x%X is 0x%X.\n",  &Buffer[4], *((uint32_t*)(&Buffer[4])));

		#ifdef _TRACE_HPRPC_VERBOSE_
		logout("Wait for finished token of the sender on 0x%X.\n", &Buffer[BufferLen_wo_EndToken]);
		#endif

		//uint32_t uFinishedToken = 0xdeadbeef;
		do
		{
			Cache_inv((xdc_Ptr*) &Buffer[BufferLen_wo_EndToken], ETL, Cache_Type_ALL, CACHE_BLOCKING);
		}
		//while(0 == Buffer[BufferLen_wo_EndToken]); //todo: uFinishedToken == ... and (uint32_t)
		while(0 == (uint32_t) *((uint8_t*)(&Buffer[BufferLen_wo_EndToken])));

		#ifdef _TRACE_HPRPC_VERBOSE_
		logout("Finished token received, we can proceed ...\n");
		#endif

		Cache_inv((xdc_Ptr*) Buffer, BufferLen_wo_EndToken, Cache_Type_ALL, CACHE_BLOCKING);	//+-ETL: The first bytes were already made sure to have been received
	}

	//Remember that we possibly modified memory and are not initialized anymore
	mbInitNecessary = true;

	//While an incoming StoreImageRequest is received (which consists out of several incoming frames) no new command is expected
	if(true == mbReceivingAnImageRequest)
	{
		Process_HPRPC_StoreImage_Request(Buffer, BufferLen_wo_EndToken);
	}
	else
	{
		//This must be a new command with HPRPC header. Parse the header to find out about Request/Response/OpCode

		//Read first word from the protocol
		uint16_t iFirstHPRPCWord = *((uint16_t*)Buffer);

		//Parse and process
		if(0 != (iFirstHPRPCWord & 0x8000))
		{
			//Received a response on the target. This is not possible. In the current application version
			//no requests will be sent from the target to the PC, hence no response will be received on
			//the target.
			logout("ERROR:  HPRPCMalformedException()\n");
			exit(0);
		}

		//Check wether a new command arrives while a store image request was still in progress
		if((HPRPC_OP_STORE_IMG != iFirstHPRPCWord) && (true == mbReceivingAnImageRequest))
		{
			logout("ERROR: Received command with OpCode %u while an image store request (OpCode %u) was still in progress.\n", iFirstHPRPCWord, HPRPC_OP_STORE_IMG);
			logout("ERROR:  HPRPCMalformedException()\n");
			exit(0);
		}

		//Dump request memory (we do this here because we don't want to dump bulk image data)
		#ifdef _TRACE_HPRPC_VERBOSE_
		if(HPRPC_OP_STORE_IMG != iFirstHPRPCWord)	//don't log image store commands (also not the first image buffer)
		{
			DumpMemory("HPRPC_Req", Buffer, BufferLen_wo_EndToken);
		}
		#endif

		//This is a hprpc request (no response). The first word is the OpCode.
		//Act according to the OpCode.
		switch(iFirstHPRPCWord)
		{

			case HPRPC_OP_STORE_IMG:
				//Retreive image and bounding box data
				#ifdef _TRACE_HPRPC_
				logout("### Rcv StoreImage request\n");
				#endif
				Process_HPRPC_StoreImage_Request(Buffer, BufferLen_wo_EndToken);
				break;

			case HPRPC_OP_CALC_SSD_JAC_HESS:
				//Calculate SSD, Jacobian and Hessian
				#ifdef _TRACE_HPRPC_
				logout("### Rcv CalcSSDJacJess request\n");
				#endif
				Process_HPRPC_CalcSSDJacHess_Request(Buffer, BufferLen_wo_EndToken);
				break;

			case HPRPC_OP_CALC_SSD:
				//Calcuclate SSD only (used during Armijo line search)
				#ifdef _TRACE_HPRPC_
				logout("### Rcv CalcSSD request\n");
				#endif
				Process_HPRPC_CalcSSD_Request(Buffer, BufferLen_wo_EndToken);
				break;

			case HPRPC_OP_CALC_FINISHED:
				//We are allowed to free (zero out) memory and to prepare for another upcoming calculation
				#ifdef _TRACE_HPRPC_
				logout("### Rcv CalcFinished request\n");
				#endif
				Process_HPRPC_CalcFinished_Request(Buffer, BufferLen_wo_EndToken);
				break;

			default:
				logout("ERROR: Ignoring unknown HPRPC OpCode %u.\n", (uint32_t) iFirstHPRPCWord);
		}
	}
}

/**
* Parse an incoming HPRPC_OP_STORE_IMG HPRPC request, execute the command. Then assemble and send an HPRPC response message back to the client PC.
*/
void CTargetConnector::Process_HPRPC_StoreImage_Request(const uint8_t* Buffer, const uint32_t BufferLen)
{
	//Store image data and bounding boxes to RAM

	//Decide depending on the last incoming request whether this is a first or a subsequent store image call
	if(false == mbReceivingAnImageRequest)
	{
		//First incoming frame of a HPRPC store image request
		mbReceivingAnImageRequest = mStoreImgRequest.AssignFirstRcvBuffer(Buffer, BufferLen);
	}
	else
	{
		mbReceivingAnImageRequest = mStoreImgRequest.AssignSubsequentRcvBuffer(Buffer, BufferLen);
	}

	//When the mbReceivingAnImageRequest was set to false this means: All data avilable (not listening anymore).
	//Now an answer is expected by the caller. Furthermore the call can be processed (as all data is available).
	if(false == mbReceivingAnImageRequest)
	{
		uint32_t TimeForAddMargins=0;
		uint32_t TimeForPyramid=0;

		#ifdef _TRACE_PYRAMID_TIME_
			//Generate pyramid before sending ImageRcvNote to caller when we wnat to pass the timing values back for tracing purposes

			//Now we have all data we can generate the multilevel pyramid.
			mStoreImgRequest.GenerateMultilevelPyramid(TimeForAddMargins, TimeForPyramid);
		#endif

		//Send an answer to the remote peer about the fact that all images were stored correctly.
		//Assemble response (Returncode=ok)
		CHPRPCResponseStoreImg oResponse;

		oResponse.AssignValuesToBeSent(mStoreImgRequest.GetCallIndex(), TimeForAddMargins, TimeForPyramid);
		oResponse.SendToOtherPeer(*m_pHPRCPServer);
		#ifdef _TRACE_HPRPC_
		logout("### Snd StoreImage OK notification\n");
		#endif

		#ifndef _TRACE_PYRAMID_TIME_
			//See above (ifdef trace_pyramid), otherwise - if not defined - generate the pyramid here in parallel to the PC peer operation
			//Now we have all data we can generate the multilevel pyramid.
			mStoreImgRequest.GenerateMultilevelPyramid(TimeForAddMargins, TimeForPyramid);
		#endif
	}
	else
	{
		//The ethernet version was capable of receiving several packets. The current version should be compatible too if one extends the code a bit.
		//Here an IRQ must be set and in the host application the IRQ must be waited for. (There's allready a comment at that place.) One should
		//think about the performance for this procedure, do we loose parallelism ?
		//
		//If this will be implemented giTVecStaticMemorySize and giRVecStaticMemorySize should be big enough to fill all available DDR3 space.

		//else: Continue listening, data cannot be processed yet, tell client that we can receive more data
		logout("ERROR: HUGE MULTIPACKET TRANSFERS NOT IMPLEMENTED.\n");
		//we should give the sender an empty irq-answer here to tell him to continue to send.

		exit(0);
	}

}


/**
* Parse an incoming HPRPC_OP_CALC_SSD_JAC_HESS HPRPC request, execute the command. Then assemble and send an HPRPC response message back to the client PC.
*/
void CTargetConnector::Process_HPRPC_CalcSSDJacHess_Request(const uint8_t* Buffer, const uint32_t BufferLen)
{
	//Calculate SSD, Jacobian and Hessian
	CHPRPCRequestCalcSSDJacHess oRequest(CHPRPCRequestCalcSSDJacHess::CALC_SSD_JAC_HESS);
	oRequest.AssignRcvBuffer(Buffer, BufferLen);

	HPRPC_HDR_Request_CalcSSDJacHess* pHeader = oRequest.GetPayloadHeader();
	t_reg_real JD[3];
	t_reg_real JD2[9];

	t_reg_real fSSD = CalcSSDJacHess(pHeader->Level, pHeader->w, JD, JD2, pHeader->NumberOfCores);

	//Assemble response (Returncode=ok, SSD, JD, JD2)
	CHPRPCResponseCalcSSDJacHess oResponse;
	oResponse.AssignValuesToBeSent(oRequest.GetCallIndex(), fSSD, JD, JD2);
	oResponse.SendToOtherPeer(*m_pHPRCPServer);

	#ifdef _TRACE_HPRPC_
	logout("### Snd CalcSSDJacHess response\n");
	#endif
}

/**
* Parse an incoming HPRPC_OP_CALC_SSD request, execute the command. Then assemble and send an HPRPC response message back to the client PC.
*/
void CTargetConnector::Process_HPRPC_CalcSSD_Request(const uint8_t* Buffer, const uint32_t BufferLen)
{
	//Calculate SSD
	CHPRPCRequestCalcSSDJacHess oRequest(CHPRPCRequestCalcSSDJacHess::CALC_SSD_ONLY);
	oRequest.AssignRcvBuffer(Buffer, BufferLen);

	HPRPC_HDR_Request_CalcSSDJacHess* pHeader = oRequest.GetPayloadHeader();
	t_reg_real fSSD = CalcSSD(pHeader->Level, pHeader->w, pHeader->NumberOfCores);

	//Assemble response (Returncode=ok, SSD)
	CHPRPCResponseCalcSSD oResponse;
	oResponse.AssignValuesToBeSent(oRequest.GetCallIndex(), fSSD);
	oResponse.SendToOtherPeer(*m_pHPRCPServer);
	#ifdef _TRACE_HPRPC_
	logout("### Snd CalcSSD response\n");
	#endif
}

/**
* Parse an incoming HPRPC_OP_CALC_FINISHED request, execute the command.
*/
void CTargetConnector::Process_HPRPC_CalcFinished_Request(const uint8_t* Buffer, const uint32_t BufferLen)
{
	//Discard (only virtually allocated - in the background it is a static buffer) memory
	//Zero out image memory to have prepared bounding boxes for the next calculation
	//Performance doesn't matter here as this takes place between two independent calculations.
	Reinitialize();
}

t_reg_real CTargetConnector::CalcSSDJacHess(const uint32_t level, const t_reg_real w[3], t_reg_real JD[3], t_reg_real JD2[9], const uint32_T number_of_cores)
{
	//Extract values from multidimensional matlab arrays
	uint32_t BoundBox[4];
	uint32_t MarginAddon[3];
	t_reg_real DSPRange[4];
	uint32_t TOffset;
	uint32_t ROffset;
	ExtractLevel(level, BoundBox, MarginAddon, DSPRange, TOffset, ROffset);

	emxArray_uint8_T *TvecPtr=mStoreImgRequest.GetTVec()->GetMatlabArrayPtr();
	emxArray_uint8_T *RvecPtr=mStoreImgRequest.GetRVec()->GetMatlabArrayPtr();

	//Pass on call to matlab generated code
	t_reg_real rRetVal=0;
	jacobian(
		w,
		BoundBox,
		MarginAddon,
		DSPRange,
		TvecPtr,
		TOffset,
		RvecPtr,
		ROffset,
		mStoreImgRequest.GetD() / (0x1 << level),	//use d of this level (d divided by 2^level)
		&rRetVal,
		JD,
		JD2,
		number_of_cores
		);

	return rRetVal;
}

t_reg_real CTargetConnector::CalcSSD(const uint32_t level, const t_reg_real w[3], const uint32_T number_of_cores)
{
	//Extract values from multidimensional matlab arrays
	uint32_t BoundBox[4];
	uint32_t MarginAddon[3];
	t_reg_real DSPRange[4];
	uint32_t TOffset;
	uint32_t ROffset;
	ExtractLevel(level, BoundBox, MarginAddon, DSPRange, TOffset, ROffset);

	emxArray_uint8_T *TvecPtr=mStoreImgRequest.GetTVec()->GetMatlabArrayPtr();
	emxArray_uint8_T *RvecPtr=mStoreImgRequest.GetRVec()->GetMatlabArrayPtr();

	//Pass on call to matlab generated code
	t_reg_real rRetVal = ssd(
		w,
		BoundBox,
		MarginAddon,
		DSPRange,
		TvecPtr,
		TOffset,
		RvecPtr,
		ROffset,
		mStoreImgRequest.GetD() / (0x1 << level),	//use d of this level (d divided by 2^level)
		number_of_cores
		);

	return rRetVal;
}

/**
* Extract values for one specific level from the multilevel-enhanced arrays (which are 2 dimensional with the level as one additional parameter)
*/
void CTargetConnector::ExtractLevel(const uint32_t level, uint32_t BoundBox[4], uint32_t
             MarginAddon[3], t_reg_real DSPRange[4], uint32_t &TOffset, uint32_t &ROffset)
{
	//BoundBox
	for(int i=0; i<4; i++)	//todo: sizeof(a)/sizeof(b) ... also below
	{
		TMatlabArray_UInt32* pBoundBoxPerLvl = mStoreImgRequest.GetBoundBoxPerLvl();
		BoundBox[i] = pBoundBoxPerLvl->GetCMemoryArrayPtr()[pBoundBoxPerLvl->GetMatlabArrayPtr()->size[0] * i + level];
	}

	//MarginAddon
	for(int i=0; i<3; i++)
	{
		TMatlabArray_UInt32* pMarginAddonPerLvl = mStoreImgRequest.GetMarginAdditionPerLvl();
		MarginAddon[i] = pMarginAddonPerLvl->GetCMemoryArrayPtr()[pMarginAddonPerLvl->GetMatlabArrayPtr()->size[0] * i + level];
	}

	//DSPRange
	for(int i=0; i<4; i++)
	{
		TMatlabArray_Reg_Real* pDSPRangePerLvl = mStoreImgRequest.GetDSPRangePerLvl();
		DSPRange[i] = pDSPRangePerLvl->GetCMemoryArrayPtr()[pDSPRangePerLvl->GetMatlabArrayPtr()->size[0] * i + level];
	}

	//TOffset
	TMatlabArray_UInt32* pTLvlPtrs = mStoreImgRequest.GetTLvlPtrs();
	TOffset = pTLvlPtrs->GetCMemoryArrayPtr()[level] - 1;	//start offset (-1 because c++ uses 0, matlab 1 as start index)

	//ROffset
	TMatlabArray_UInt32* pRLvlPtrs = mStoreImgRequest.GetRLvlPtrs();
	ROffset = pRLvlPtrs->GetCMemoryArrayPtr()[level] - 1;	//start offset (-1 because c++ uses 0, matlab 1 as start index)
}
