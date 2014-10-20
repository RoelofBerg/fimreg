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

#include <xdc/runtime/Timestamp.h>

#include "CHPRPCRequestStoreImg.h"
#include "HPRPCCommon.h"
#include "generatePyramidPC.h"

#include <xdc/cfg/global.h>
#include <xdc/runtime/Memory.h>
#include "cache_handling.h"
#include "mcip_process.h"

//Memory for some small Matlab arrays (filled here with data and read out in GeneratePyramid.cpp)
static const int32_t MAX_LEVEL_COUNT = 20;			//reserve enough memory for 20 levels of multilevel pyramid ... (at boot time)
const uint32_T gpTLvlPtrsItemCount[] = {MAX_LEVEL_COUNT, 2};	//TODO: OR VICE VERSA ???
const uint32_t giTLvlPtrsStaticMemorySize = sizeof(uint32_t) * 2 * MAX_LEVEL_COUNT;
uint32_t gpTLvlPtrsStaticMemory[giTLvlPtrsStaticMemorySize];

const uint32_T gpRLvlPtrsItemCount[] = {MAX_LEVEL_COUNT, 2};
const uint32_t giRLvlPtrsStaticMemorySize = sizeof(uint32_t) * 2 * MAX_LEVEL_COUNT;
uint32_t gpRLvlPtrsStaticMemory[giRLvlPtrsStaticMemorySize];

const uint32_T gpBoundBoxPerLvItemCount[] = {MAX_LEVEL_COUNT, 4};
const uint32_t giBoundBoxPerLvlStaticMemorySize = sizeof(uint32_t) * 4 * MAX_LEVEL_COUNT;
uint32_t gpBoundBoxPerLvlStaticMemory[giBoundBoxPerLvlStaticMemorySize];

const uint32_T gpDSPRangePerLvlItemCount[] = {MAX_LEVEL_COUNT, 4};
const uint32_t giDSPRangePerLvlStaticMemorySize = sizeof(float) * 4 * MAX_LEVEL_COUNT;
float gpDSPRangePerLvlStaticMemory[giDSPRangePerLvlStaticMemorySize];

const uint32_T gpMarginAdditionPerLvlItemCount[] = {MAX_LEVEL_COUNT, 3};
const uint32_t giMarginAdditionPerLvlStaticMemorySize = sizeof(uint32_t) * 3 * MAX_LEVEL_COUNT;
uint32_t gpMarginAdditionPerLvlStaticMemory[giMarginAdditionPerLvlStaticMemorySize];

//Image memory allocated on dedicated memory
const uint32_t giTVecStaticMemorySize = 0x4200000;
//ToDo: const uint32_t giTVecStaticMemorySize = 0x12C00000;		//300 MB (R+T+SockBuf should fill up the .far address space (just set it too high and read what's the 'max hole' message in the linker error. Then divide this space between giTVecStaticMemorySize and giRVecStaticMemorySize))
#pragma DATA_SECTION(".far:PICTUREBUFF");
#pragma DATA_ALIGN(128);
uint8_t gpTVecStaticMemory[giTVecStaticMemorySize];

const uint32_t giRVecStaticMemorySize = 0x1e00000;
//ToDo: const uint32_t giRVecStaticMemorySize = 0xC800000;		//200 MB (See comment of giTVecStaticMemorySize)
#pragma DATA_SECTION(".far:PICTUREBUFF");
#pragma DATA_ALIGN(128);
uint8_t gpRVecStaticMemory[giRVecStaticMemorySize];

//Global marker if new image data is available (then all cores must invalidate the image data in the cache)
uint32_t g_NewImageDataArrived=0;

const double cd1MB = 1024.0 * 1024.0;

CHPRPCRequestStoreImg::CHPRPCRequestStoreImg(void)
: 	miTSizeWithPyramid(0),
	miTVecRcvInIndex(0),
	miTVecRcvOutIndex(0),
	miRSizeWithPyramid(0),
	miRVecRcvInIndex(0),
	miRVecRcvOutIndex(0),
	m_TVec(gpTVecStaticMemory, giTVecStaticMemorySize),	//most image space (n levels + worst-case-image-data + margin)
	m_RVec(gpRVecStaticMemory, giRVecStaticMemorySize),	//less image space (n levels of 1/4 of R when 4 DSPs are used)
	m_TLvlPtrs(gpTLvlPtrsStaticMemory, giTLvlPtrsStaticMemorySize, gpTLvlPtrsItemCount, 2), //levelcount * 2
	m_RLvlPtrs(gpRLvlPtrsStaticMemory, giRLvlPtrsStaticMemorySize, gpRLvlPtrsItemCount, 2), //levelcount * 2
	m_BoundBoxPerLvl(gpBoundBoxPerLvlStaticMemory, giBoundBoxPerLvlStaticMemorySize, gpBoundBoxPerLvItemCount, 2), //levelcount * 4
	m_DSPRangePerLvl(gpDSPRangePerLvlStaticMemory, giDSPRangePerLvlStaticMemorySize, gpDSPRangePerLvlItemCount, 2), //levelcount * 4
	m_MarginAdditionPerLvl(gpMarginAdditionPerLvlStaticMemory, giMarginAdditionPerLvlStaticMemorySize, gpMarginAdditionPerLvlItemCount, 2),  //levelcount * 3
	miTTotalMemory(0),
	m_uDimension(0),
	m_uLevelAmount(0),
	m_uNumberOfCores(0)
{

}

CHPRPCRequestStoreImg::~CHPRPCRequestStoreImg(void)
{
	//dtor unused in current design
	DeallocateImageMemoryIfAny();
}

/**
 * Reinitialize memory at boot time and after the calculation has been finished.
 *
 * We will not allocate memory on the target system (e.g. to avoid heap fragmentation as the system might run for years and
 * no MMU is present). Hence the operations of discarding an old and creating a new instance can be executed here. The result
 * is the same as if the caller would drop an old and create a new instance.
 */
void CHPRPCRequestStoreImg::Reinitialize()
{
	//Discard (only virtually allocated - in the background it is a static buffer) memory
	DeallocateImageMemoryIfAny();

	//Reinitialize matlab arrays
	/*
	m_TLvlPtrs
	m_RLvlPtrs
	m_BoundBoxPerLvl
	m_DSPRangePerLvl
	m_MarginAdditionPerLvl
	*/

	//Initialize members
	miTVecRcvInIndex = 0;
	miTVecRcvOutIndex = 0;
	miRVecRcvInIndex = 0;
	miRVecRcvOutIndex = 0;
	miTTotalMemory = 0;
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
		logout("ERROR:  InvalidBufferException(5)\n");
		exit(0);
	}
	#endif

	return (HPRPC_HDR_Request_StoreImg*)pHeader;
}

/**
* Return a pointer to the (part of the) T image as bytearray. (Exception if no image data assigned)
*/
CMatlabArray* CHPRPCRequestStoreImg::GetTVec()
{
	return &m_TVec;
}

/**
* Return a pointer to the (part of the) R image as bytearray. (Exception if no image data assigned)
*/
CMatlabArray* CHPRPCRequestStoreImg::GetRVec()
{
	return &m_RVec;
}

TMatlabArray_UInt32* CHPRPCRequestStoreImg::GetTLvlPtrs()
{
	return &m_TLvlPtrs;
}

TMatlabArray_UInt32* CHPRPCRequestStoreImg::GetRLvlPtrs()
{
	return &m_RLvlPtrs;
}

TMatlabArray_UInt32* CHPRPCRequestStoreImg::GetBoundBoxPerLvl()
{
	return &m_BoundBoxPerLvl;
}

TMatlabArray_Reg_Real*  CHPRPCRequestStoreImg::GetDSPRangePerLvl()
{
	return &m_DSPRangePerLvl;
}

TMatlabArray_UInt32* CHPRPCRequestStoreImg::GetMarginAdditionPerLvl()
{
	return &m_MarginAdditionPerLvl;
}

uint32_t CHPRPCRequestStoreImg::GetD()
{
	return m_uDimension;
}


/**
* Used when receiving an incoming command.
*
* This must be called for the first incoming data packet. On the target each frame will be processed independently.
* A packet will have a size between 536 (mss, minimum segment size) and MTU (max. 1500) bytes. For the other HPRPC
* commands used in this application one received frame will contain all data. In that case no copy of the data will be made.
* However for the store image command data will come in several parts. Hence data will be stored into a buffer allocated by
* this class. (Furthermore RLE decompression will be performed here during data arrival - while copying the data to the
* receive memory buffer).
*
* Careful: Image data buffers will be freed when this object is destructed.
*
* Call AssignFirstRcvBuffer() for the first incoming frame. Call AssignSubsequentRcvBuffer() for all subsequent frames. If any
* of both commands returns true this indicates that the stream hasn't been finished yet and more calls are necessary. If one of
* both functions return false this means that dara receiving has been finished and the data can be used.
*
* Both functions set some pointers to the right places in the input data buffer. Use the accessor methods
* to obtain the pointers afterwards.
*
* Returns the call index of the incoming call.
*
* CAREFUL: WILL NOT COPY THE BUFFER (for execution speed reasons). Hence this object
* must be deleted before the receive buffer is freed.
*/
bool CHPRPCRequestStoreImg::AssignFirstRcvBuffer(const uint8_t* Buffer, const uint32_t BufferLen)
{
	//We need to copy the header for 8-byte-struct aligning (ToDo: This was necessary in the ethernet version. Byte-Align-Copying might have become unnecessary by the PCIe DMA layout.)
	const uint32_t iSizeOfAllHeaders = sizeof(HPRPC_HDR_Request) + sizeof(HPRPC_HDR_Request_StoreImg);
	uint8_t* pHeaderCopy = CHPRPCCommand::AllocateBuffer(iSizeOfAllHeaders);
	memcpy(pHeaderCopy, Buffer, iSizeOfAllHeaders);

	//rbe todo: HeaderCopy was necessary in ethernet version when the IP-Buffers were not 8-byte-aligned.
	//In the PCIe-Version we could 8-byte-align the data ourselves and then we don't need to copy ...

	//Assign HPRPC header
	//It is ok to pass the non-junct parameters pHeaderCopy and BufferLen as BufferLen is only used to
	//calculate the return value
	uint32_t iPayloadLen = CHPRPCRequest::AssignRcvBuffer(pHeaderCopy, BufferLen);
	HPRPC_HDR_Request_StoreImg* pPayloadHeader = GetPayloadHeader();

	//Check received buffer size
	#ifdef _DO_ERROR_CHECKS_
	if(sizeof(HPRPC_HDR_Request_StoreImg) > iPayloadLen)
	{
		logout("ERROR:  HPRPCMalformedException()\n");
		exit(0);
	}
	#endif

	//Allocate memory for the incoming images (as a matlab array)
	const uint32_t iRightMargin = pPayloadHeader->MarginAddon[0];
	const uint32_t iTopMargin = pPayloadHeader->MarginAddon[1];
	const uint32_t iBottomMargin = pPayloadHeader->MarginAddon[2];
	const uint32_t iImageWidth = pPayloadHeader->BoundBox[1]-pPayloadHeader->BoundBox[0]+1;
	const uint32_t iImageHeight = pPayloadHeader->BoundBox[3]-pPayloadHeader->BoundBox[2]+1;


	miTTotalMemory = pPayloadHeader->TSinglePartLen		//memory including zeroed bounding box
		+ iTopMargin*iImageWidth
		+ iBottomMargin*iImageWidth
		+ iRightMargin*iTopMargin
		+ iRightMargin*iImageHeight
		+ iRightMargin*iBottomMargin
		;

	#ifdef _TRACE_ADD_MARGINS_
	logout("iRightMargin=%u\n", iRightMargin);
	logout("iTopMargin=%u\n", iTopMargin);
	logout("iBottomMargin=%u\n", iBottomMargin);
	logout("iImageWidth=%u\n", iImageWidth);
	logout("iImageHeight=%u\n", iImageHeight);
	logout("miTTotalMemory=%u\n", miTTotalMemory);
	#endif

	#if defined(_DO_ERROR_CHECKS_)
	if(pPayloadHeader->TSinglePartLen != iImageWidth*iImageHeight)
	{
		logout("ERROR: T data should be %d bytes but was %d bytes.\n", iImageWidth*iImageHeight, pPayloadHeader->TSinglePartLen);
	}
	#endif

	//Max. Speicherbedarf nach geometrischer Reihe.
	miTSizeWithPyramid = (miTTotalMemory + (uint32_t)ceil((t_reg_real)miTTotalMemory / 3.0F)) + 1U;
	#ifdef _TRACE_PAPER_FIGURES_
		logout("T pyramid size: %d bytes.\n", miTSizeWithPyramid);
	#endif
	if(giTVecStaticMemorySize < miTSizeWithPyramid)
	{
		//Note: This does not mean that there is no more memory, it does only mean that the memory we defined is too small.
		//Maybe by reconfiguring the system (or the partitioning between T-Img and R-Img) one can provide more memory ...
		logout("ERROR: Size of T image is bigger than the available space for it in DSP memory. (%f MByte > %f MByte)\n", ((double)miTSizeWithPyramid)/cd1MB, ((double)giTVecStaticMemorySize)/cd1MB);
	}
	m_TVec.Reinitialize(miTSizeWithPyramid);
	//logout("STORE total memory %u\n", miTTotalMemory);
	miTVecRcvInIndex = 0;	//Read pointer in compressed data
	miTVecRcvOutIndex = 0;	//Write pointer in decompressed data

	//Max. Speicherbedarf nach geometrischer Reihe.
	miRSizeWithPyramid = (pPayloadHeader->RSinglePartLen + (uint32_t)ceil((t_reg_real)pPayloadHeader->RSinglePartLen / 3.0F)) + 1U;
	#ifdef _TRACE_PAPER_FIGURES_
		logout("R pyramid size: %d bytes.\n", miRSizeWithPyramid);
	#endif
	if(giRVecStaticMemorySize < miRSizeWithPyramid)
	{
		//Note: See note about 15 lines before at T-Image.
		logout("ERROR: Size of R image is bigger than the available space for it in DSP memory. (%f MByte > %f MByte)\n", ((double)miRSizeWithPyramid)/cd1MB, ((double)giRVecStaticMemorySize)/cd1MB);
	}
	m_RVec.Reinitialize(miRSizeWithPyramid);
	miRVecRcvInIndex = 0;	//Read pointer in compressed data
	miRVecRcvOutIndex = 0;	//Write pointer in decompressed data

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
	uint32_t d=pPayloadHeader->d;
	uint32_t lev=pPayloadHeader->levels;
	logout("StoreImg Request D=%u L=%u BB=[%u, %u, %u, %u] DB=[%f, %f, %f, %f]\n",
			d, lev, b1, b2, b3, b4, (double)d1, (double)d2, (double)d3, (double)d4);
	#endif
	#ifdef _DO_ERROR_CHECKS_
	if(MAX_LEVEL_COUNT < pPayloadHeader->levels)
	{
		printf("ERROR: Max. %d levels or less are supported (requested: %d, can be easily extended by changing MAX_LEVEL_COUNT)\n", MAX_LEVEL_COUNT, pPayloadHeader->levels);
	}
	#endif

	//Assign top margin to T image
	m_CImageBuffer.SetMargins(iRightMargin, iTopMargin, iBottomMargin, iImageWidth);
	miTVecRcvOutIndex = m_CImageBuffer.PushHeaderMargin(m_TVec.GetCMemoryArrayPtr(), miTTotalMemory);
	#ifdef _TRACE_ADD_MARGINS_
	logout("Added top margin: miTVecRcvOutIndex=%u\n", miTVecRcvOutIndex);
	#endif

	//Virtually pass the rest of the data of the first packet as a second packet
	return AssignSubsequentRcvBuffer((uint8_t*)Buffer + sizeof(HPRPC_HDR_Request) + sizeof(HPRPC_HDR_Request_StoreImg), iPayloadLen - sizeof(HPRPC_HDR_Request_StoreImg));
}

/**
* Receive a subsequent buffer. For documentation see AssignFirstRcvBuffer().
*/
bool CHPRPCRequestStoreImg::AssignSubsequentRcvBuffer(const uint8_t* Buffer, const uint32_t BufferLen)
{
	HPRPC_HDR_Request_StoreImg* pPayloadHeader = GetPayloadHeader();

	uint32_t iTRcvLeft = pPayloadHeader->TSinglePartLen - miTVecRcvInIndex;
	uint32_t iTToReceiveLen=0;
	if(iTRcvLeft>0)
	{
		//Still not all T data received

		//How many bytes are on the data stream that are meant for image T ?
		iTToReceiveLen = (BufferLen<=iTRcvLeft) ? BufferLen : iTRcvLeft;

		//RLE decoding (stateful and directly from the oepratign systems network buffer into a matlab array without wasting any copy operations)
		const uint32_t iTDeomprLen = m_CImageBuffer.PushImageDataWithMargins(m_TVec.GetCMemoryArrayPtr() + miTVecRcvOutIndex,
														 miTTotalMemory - miTVecRcvOutIndex,
														 Buffer, iTToReceiveLen);

		miTVecRcvInIndex+=iTToReceiveLen;
		miTVecRcvOutIndex += iTDeomprLen;

		//Last buffer of T received ? Then add bottom margin
		if(miTVecRcvInIndex == pPayloadHeader->TSinglePartLen)
		{
			#ifdef _TRACE_ADD_MARGINS_
			logout("Added data and right margin: miTVecRcvOutIndex=%u\n", miTVecRcvOutIndex);
			#endif

			const uint32_t iFooterLen = m_CImageBuffer.PushFooterMargin(m_TVec.GetCMemoryArrayPtr()+miTVecRcvOutIndex, miTTotalMemory - miTVecRcvOutIndex);
			miTVecRcvOutIndex += iFooterLen;

			#ifdef _TRACE_ADD_MARGINS_
			logout("Added footer margin: miTVecRcvOutIndex=%u\n", miTVecRcvOutIndex);
			#endif
		}
	}

	//Calculate buffer left for R image data
	const uint8_t* RBuffer = &Buffer[iTToReceiveLen];
	const uint32_t RBufferLen = BufferLen-iTToReceiveLen;

	//If RBuflen is >0 then some data for image R is present
	//(Either because the above if wasn't entered or because it was entered, processed
	//the final rest of the R data. In the latter case RBuffer points to the beginning
	//of the T image data and RBufferLen gives the length of the T image data from the
	//bottom of the incoming package.)
	bool bRcvStillDatatoCome = true;
	if(RBufferLen>0)
	{

		const uint32_t iRRcvLeft = pPayloadHeader->RSinglePartLen - miRVecRcvInIndex;
		//How many bytes are on the data stream that are meant for image R ?
		const uint32_t iRToReceiveLen = (RBufferLen<=iRRcvLeft) ? RBufferLen : iRRcvLeft;

		//RLE decoding (stateful and directly from the operating systems network buffer into a matlab array without wasting any copy operations)
		const uint32_t iRDeomprLen = m_CImageBuffer.PushImageData(m_RVec.GetCMemoryArrayPtr() + miRVecRcvOutIndex,
														 pPayloadHeader->RSinglePartLen - miRVecRcvOutIndex,
														 RBuffer, iRToReceiveLen);
		//RLE decoding (stateful and directly from the operating systems network buffer into a matlab array without wasting any copy operations)
		miRVecRcvInIndex+=iRToReceiveLen;
		miRVecRcvOutIndex+=iRDeomprLen;

		//More data passed than specified in the length fields ?
		#ifdef _DO_ERROR_CHECKS_
		if(iRRcvLeft < RBufferLen)
		{
			logout("ERROR:  InvalidBufferException(6)  [%d < %d]\n", iRRcvLeft, RBufferLen);
			exit(0);
		}
		#endif

		//All R data received ?
		bRcvStillDatatoCome = (iRRcvLeft != RBufferLen);
	}

	//Last buffer received ? Then cache write back the matlab arrays containing the new image data (and set g_NewImageDataArrived signal so the other cores will cache invalidate this memory).
	if(false == bRcvStillDatatoCome)
	{
		#ifdef _DO_ERROR_CHECKS_
		if(giTVecStaticMemorySize < miTVecRcvOutIndex)
		{
			logout("Out of memory for T image data.\n");
			exit(0);
		}
		if(giRVecStaticMemorySize < miRVecRcvOutIndex)
		{
			logout("Out of memory for R image data.\n");
			exit(0);
		}
		if(miTTotalMemory != miTVecRcvOutIndex)
		{
			logout("ERROR: StoreImg 1 (%d, %d)\n", miTTotalMemory, miTVecRcvOutIndex);
			//The following line wasn't printed to RS232. ToDo: Why ? Is exit(0) too fast ?
			//logout("ERROR: Less/More image data of T initialized than it should be (should be %u bytes, but was %u bytes)\n", miTTotalMemory, miTVecRcvOutIndex);
			exit(0);
		}
		if(miTVecRcvOutIndex > m_TVec.GetMatlabArrayPtr()->allocatedSize)
		{
			logout("ERROR: StoreImg 2 (%d, %d)\n", miTVecRcvOutIndex, m_TVec.GetMatlabArrayPtr()->allocatedSize);
			//The following line wasn't printed to RS232. ToDo: Why ? Is exit(0) too fast ?
			//logout("ERROR: Matlab array content more than it should be (should be %u bytes, but was %i bytes)\n", miTVecRcvOutIndex, m_TVec.GetMatlabArrayPtr()->allocatedSize);
			exit(0);
		}
		if(miRVecRcvOutIndex > m_RVec.GetMatlabArrayPtr()->allocatedSize)
		{
			logout("ERROR: StoreImg 3 (%d, %d)\n", miRVecRcvOutIndex, m_RVec.GetMatlabArrayPtr()->allocatedSize);
			//The following line wasn't printed to RS232. ToDo: Why ? Is exit(0) too fast ?
			//logout("ERROR: Matlab array content more than it should be (should be %u bytes, but was %i bytes)\n", miRVecRcvOutIndex, m_RVec.GetMatlabArrayPtr()->allocatedSize);
			exit(0);
		}

		#endif

		//debug
		#ifdef _TRACE_OUTPUT_
		uint8_t* p=m_TVec.GetCMemoryArrayPtr();
		//uint8_t* p=mStoreImgRequest.GetTVec()->GetCMemoryArrayPtr();
		uint32_t iWidth = (GetPayloadHeader()->BoundBox[1]+1) - GetPayloadHeader()->BoundBox[0];
		uint32_t iHeight = (GetPayloadHeader()->BoundBox[3]+1) - GetPayloadHeader()->BoundBox[2];
		uint32_t iA=iWidth*(iHeight/4)+(iWidth/4)-1;
		uint32_t iB=iWidth*(iHeight/2)+(iWidth*3/4)-1;
		logout("f(0), f(1) ... f(0x%x), f(0x%x) ... f(0x%x), f(0x%x)\n", iA, iA+1, iB-1, iB);
		logout("T: %.2x %.2x ... %.2x %.2x ... %.2x %.2x\n",
				p[0], p[1], p[iA], p[iA+1], p[iB-1], p[iB]);

		p=m_RVec.GetCMemoryArrayPtr();
		iWidth = uint32_t(pPayloadHeader->DSPResponsibilityBox[1] - pPayloadHeader->DSPResponsibilityBox[0]);
		iHeight = uint32_t(pPayloadHeader->DSPResponsibilityBox[3] - pPayloadHeader->DSPResponsibilityBox[2]);
		iA=iWidth*(iHeight/4)+(iWidth/4)-1;
		iB=iWidth*(iHeight/2)+(iWidth*3/4)-1;
		logout("R: %.2x %.2x ... %.2x %.2x ... %.2x %.2x\n",
				p[0], p[1], p[iA], p[iA+1], p[iB-1], p[iB]);
		#endif

		//Remember data for the multilevel pyramid generation (I don't want to access the header copy there as I saw a bug once and because network activity takes place between this line and the call to GeneratePyramid() when the ImgRcvOk packet is activated in the compile config.)
		PrepareMultilevelPyramidGeneration(pPayloadHeader);
	}

	//Report to caller whether all image data has been received.
	return bRcvStillDatatoCome;
}

/**
 * Remember data for the multilevel pyramid generation (In the former Ethernet version it was not possible to access the header copy. Maybe copying is no longer necessary in the PCIe DMA version.)
 */
void CHPRPCRequestStoreImg::PrepareMultilevelPyramidGeneration(HPRPC_HDR_Request_StoreImg* PayloadHeader)
{
	//Remember d and level-amount (equal for all levels)
	m_uDimension = PayloadHeader->d;
	m_uLevelAmount = PayloadHeader->levels;
	m_uNumberOfCores = PayloadHeader->NumberOfCores;

	//Dimensions arrays
	uint32_t uLevelsX2[] = {m_uLevelAmount, 2};
	uint32_t uLevelsX3[] = {m_uLevelAmount, 3};
	uint32_t uLevelsX4[] = {m_uLevelAmount, 4};

	//Reinitialize and fill matlab arrays
	m_TLvlPtrs.Reinitialize(uLevelsX2, 2);
	m_RLvlPtrs.Reinitialize(uLevelsX2, 2);
	m_BoundBoxPerLvl.Reinitialize(uLevelsX4, 2);
	m_DSPRangePerLvl.Reinitialize(uLevelsX4, 2);
	m_MarginAdditionPerLvl.Reinitialize(uLevelsX3, 2);

	//Copy values for first level to the first data set of the two dimensional matrices
	for(int i=0; i<4; i++)	//todo: sizeof(a)/sizeof(b) ... also below
	{
		m_BoundBoxPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * i] = PayloadHeader->BoundBox[i];
	}

	for(int i=0; i<4; i++)
	{
		m_DSPRangePerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * i] = PayloadHeader->DSPResponsibilityBox[i];
	}

	for(int i=0; i<3; i++)
	{
		m_MarginAdditionPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * i] = PayloadHeader->MarginAddon[i];
	}
}

/**
* Generate the multilevel pyramid. Registration will take place on the smalles image first.
*/
void CHPRPCRequestStoreImg::GenerateMultilevelPyramid(uint32_t& TimeForAddMargins, uint32_t& TimeForPyramid)
{
		#ifdef _TRACE_HPRPC_
		logout("Generating multilevel pyramid (%d levels, d=%d, dsp cores=%d).\n", m_uLevelAmount, m_uDimension, m_uNumberOfCores);
		#endif

		uint32_t time1 = Timestamp_get32();
		uint32_t time2 = Timestamp_get32();
		TimeForAddMargins = time2-time1;

		//Generate multilevel pyramid

		#ifdef _TRACE_OUTPUT_
		logout("First level received via HPRPC:\n", m_uLevelAmount);
		logout("  BoundBox_L1=[%d %d %d %d]\n",
				m_BoundBoxPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 0],
				m_BoundBoxPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 1],
				m_BoundBoxPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 2],
				m_BoundBoxPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 3]
			  );
		logout("  DSPRange_L1=[%f %f %f %f]\n",
				(double)(m_DSPRangePerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 0]),
				(double)(m_DSPRangePerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 1]),
				(double)(m_DSPRangePerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 2]),
				(double)(m_DSPRangePerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 3])
			  );
		logout("  MarginAddon_L1=[%d %d %d]\n",
				m_MarginAdditionPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 0],
				m_MarginAdditionPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 1],
				m_MarginAdditionPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 2]
			  );
		#endif

		//Generate multilevel pyramid (internally handles cachewriteback of all touched data as well as of the R and T images we received above).
		generatePyramidPC(
			m_TVec					.GetMatlabArrayPtr(),
			m_BoundBoxPerLvl		.GetMatlabArrayPtr(),
			m_MarginAdditionPerLvl	.GetMatlabArrayPtr(),
			m_RVec					.GetMatlabArrayPtr(),
			m_DSPRangePerLvl		.GetMatlabArrayPtr(),
			m_uLevelAmount			,
			m_TLvlPtrs				.GetMatlabArrayPtr(),
			miTSizeWithPyramid		,
			m_RLvlPtrs				.GetMatlabArrayPtr(),
			miRSizeWithPyramid,
			m_uNumberOfCores
			);

		UInt32 time3 = Timestamp_get32();
		TimeForPyramid = time3-time2;

		#ifdef _TRACE_OUTPUT_
		logout("Multilevel pyramid:\n");
		for(unsigned int i=0; i<m_uLevelAmount; i++)
		{
			logout("  BoundBox_L%d=[%d %d %d %d]\n", i,
					m_BoundBoxPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 0 + i],
					m_BoundBoxPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 1 + i],
					m_BoundBoxPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 2 + i],
					m_BoundBoxPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 3 + i]
				  );
			logout("  DSPRange_L%d=[%f %f %f %f]\n", i,
					(double)(m_DSPRangePerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 0 + i]),
					(double)(m_DSPRangePerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 1 + i]),
					(double)(m_DSPRangePerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 2 + i]),
					(double)(m_DSPRangePerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 3 + i])
				  );
			logout("  MarginAddon_L%d=[%d %d %d]\n", i,
					m_MarginAdditionPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 0 + i],
					m_MarginAdditionPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 1 + i],
					m_MarginAdditionPerLvl.GetCMemoryArrayPtr()[m_uLevelAmount * 2 + i]
				  );
			logout("  TLvlPtrs_L%d=[%d %d]\n", i,
					m_TLvlPtrs.GetCMemoryArrayPtr()[m_uLevelAmount * 0 + i],
					m_TLvlPtrs.GetCMemoryArrayPtr()[m_uLevelAmount * 1 + i]
				  );
			logout("  RLvlPtrs_L%d=[%d %d]\n", i,
					m_RLvlPtrs.GetCMemoryArrayPtr()[m_uLevelAmount * 0 + i],
					m_RLvlPtrs.GetCMemoryArrayPtr()[m_uLevelAmount * 1 + i]
				  );
		}
		#endif

		#ifdef _TRACE_HPRPC_
		logout("Multilevel pyramid generation finished.\n");
		#endif

		//Marker for all other DSP cores to cache invalidate their image data buffers (once)
		//(Cache write back was already executed during pyramid generation.)
		g_NewImageDataArrived=1;
		#ifdef _TRACE_MC_
			logout("[MAIN  ] Set g_NetImageDataArrived signal to %d.\n", g_NewImageDataArrived);
		#endif
}

/**
* This class allocates memory on the heap when receiving image data.
* This method frees this memory (if any memory has been allocated).
*
* It furthermore zeros out the memory of the template image because
* it will get a bounding box lateron. This expensive operation will
* only be called after a calculation and at boot time hence performance
* is unimportant and we use only one core.
*/
void CHPRPCRequestStoreImg::DeallocateImageMemoryIfAny()
{
	m_TVec.Reinitialize(0);
	m_RVec.Reinitialize(0);

	memset(gpTVecStaticMemory, 0, giTVecStaticMemorySize);
	Cache_wb((xdc_Ptr*) gpTVecStaticMemory, giTVecStaticMemorySize, Cache_Type_ALL, CACHE_BLOCKING);

	static bool gbBootTime = true;
	if(gbBootTime)
	{
		//Dont log at the first entry to this method (at boot time) because vuart isn't initialized at that time which screws up vuart.
		gbBootTime=false;
	}
	else
	{
		#ifdef _TRACE_OUTPUT_
		const double cd1MB = 1024.0*1024.0;
		//R doesn't have to be cleared as for R we will use no margins
		logout("======================================\nCalculation Finished, %f MByte of memory zeroized for T. (%f MByte available for R).\n======================================\n",
				(double)giTVecStaticMemorySize/cd1MB, (double)giRVecStaticMemorySize/cd1MB);
		#endif

		//Cache invalidate on all cores (except the master core)
		const uint32_t ALL_CORES_EXCEPT_MASTER=(CORE_AMOUNT-1);
		cacheinval_on_core(gpTVecStaticMemory, giTVecStaticMemorySize, ALL_CORES_EXCEPT_MASTER);
	}
}
