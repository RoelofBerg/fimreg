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

#pragma once

#include "dspreg_common.h"

#include "CHPRPCRequest.h"
#include "CImageBuffer.h"
#include "CMatlabArray.h"

/** Representation in the data stream */
struct HPRPC_HDR_Request_StoreImg
{
	uint32_t d;
	uint16_t levels;
	uint16_t NumberOfCores;
	uint32_t BoundBox[4];
	uint32_t MarginAddon[3];
	t_reg_real DSPResponsibilityBox[4];
    uint32_t TSinglePartLen;
	uint32_t RSinglePartLen;
};

//Forward declaration
class CHPRPCRequest;

/**
* Class for parsing and generation of HPRPC command 'StoreImage'.
*
* StoreImageRequest will be processed using several incoming calls. First call AssignFirstRcvBuffer(), then call AssignSubsequentRcvBuffer() on and on. If any
* of this calls will return false all data has been received. Picture data is then ready (RLE decoded in a Matlab compatible array). Picture data will vanish
* when this class will be deleted.
*/
class CHPRPCRequestStoreImg : public CHPRPCRequest
{
public:
	CHPRPCRequestStoreImg();
	virtual ~CHPRPCRequestStoreImg(void);

	bool AssignFirstRcvBuffer(const uint8_t* Buffer, const uint32_t BufferLen);
	bool AssignSubsequentRcvBuffer(const uint8_t* Buffer, const uint32_t BufferLen);
	void GenerateMultilevelPyramid(uint32_t& TimeForAddMargins, uint32_t& TimeForPyramid);
	void Reinitialize();

	HPRPC_HDR_Request_StoreImg* GetPayloadHeader();
	CMatlabArray* GetTVec();
	CMatlabArray* GetRVec();
	uint32_t GetD();
	TMatlabArray_UInt32* GetTLvlPtrs();
	TMatlabArray_UInt32* GetRLvlPtrs();
	TMatlabArray_UInt32* GetBoundBoxPerLvl();
	TMatlabArray_Reg_Real*  GetDSPRangePerLvl();
	TMatlabArray_UInt32* GetMarginAdditionPerLvl();

private:
	void DeallocateImageMemoryIfAny();
	void PrepareMultilevelPyramidGeneration(HPRPC_HDR_Request_StoreImg* PayloadHeader);

	CImageBuffer m_CImageBuffer;
	uint32_t miTSizeWithPyramid;
	uint32_t miTVecRcvInIndex;
	uint32_t miTVecRcvOutIndex;
	uint32_t miRSizeWithPyramid;
	uint32_t miRVecRcvInIndex;
	uint32_t miRVecRcvOutIndex;
	uint32_t miTTotalMemory;
	uint32_t m_uNumberOfCores;

	uint32_t m_uDimension;
	uint32_t m_uLevelAmount;

	CMatlabArray m_TVec;
	CMatlabArray m_RVec;
	TMatlabArray_UInt32 m_TLvlPtrs;
	TMatlabArray_UInt32 m_RLvlPtrs;
	TMatlabArray_UInt32 m_BoundBoxPerLvl;
	TMatlabArray_Reg_Real  m_DSPRangePerLvl;
	TMatlabArray_UInt32 m_MarginAdditionPerLvl;
};
