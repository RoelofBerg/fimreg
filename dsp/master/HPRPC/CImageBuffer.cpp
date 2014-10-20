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

/**
* Add margins while receiving a continuous pixelstream
*/

//precompiled header (MUST BE THE FIRST ENTRY IN EVERY CPP FILE)
#include "stdafx.h"

#include "dspreg_common.h"
#include "CImageBuffer.h"

/**
* constructor
*/
CImageBuffer::CImageBuffer()
: miTotalIndex(0),
  m_rightMargin(0),
  m_topMargin(0),
  m_bottomMargin(0),
  m_imageWidth(0)
{

}

/**
* destructor
*/
CImageBuffer::~CImageBuffer()
{

}

/**
* Move data from the receive to the operation memory
* Returns the amount of bytes written to OutBuffer.
* (todo: Is it a useless overhead to copy memory from the DMA receive buffer to another place ?)
*/
uint32_t CImageBuffer::PushImageData(uint8_t *OutBuffer, const uint32_t OutBufferSize, const uint8_t *InBuffer, const uint32_t InBufferSize)
{
	#ifdef _DO_ERROR_CHECKS_
	if(OutBufferSize<InBufferSize)
	{
		logout("ERROR: BUFFER OVERFLOW IN PushImageData()\n");
		exit(0);
	}
	#endif

	memcpy(OutBuffer, InBuffer, InBufferSize);
	return InBufferSize;

}

/**
* Move data from the receive to the operation memory. Adds the margins defined by SetMargins().
* (Don't forget to call PushHeaderMargin() beforehand and PushFooterMargin() afterwards)
* Returns the amount of bytes written to OutBuffer.
*/
uint32_t CImageBuffer::PushImageDataWithMargins(uint8_t *OutBuffer, const uint32_t OutBufferSize, const uint8_t *InBuffer, const uint32_t InBufferSize)
{
	uint32_t uBytesWritten = 0;
	const uint32_t uRowLen=m_imageWidth;
	const uint32_t uCurrentRowFill = miTotalIndex % uRowLen; //bytes allready processed in the current row from the runs before
	const uint32_t uCurrentRowLeft = uRowLen - uCurrentRowFill; //bytes left to finish the current row (can be up to a whole row)

	#ifdef _TRACE_ADD_MARGINS_
	logout("miTotalIndex=%d\n", miTotalIndex);
	logout("OutAddr=0x%x\n", &OutBuffer[0]);
	logout("InBufferSize=%d\n", InBufferSize);
	logout("uRowLen=%d  (and m_imageWidth)\n", uRowLen);
	logout("uCurrentRowFill=%d\n", uCurrentRowFill);
	logout("uCurrentRowLeft=%d\n", uCurrentRowLeft);
	#endif

	//Did we receive less than 2 lines ?
	if(InBufferSize < uCurrentRowLeft)
	{
		memcpy(&OutBuffer[0], &InBuffer[0], InBufferSize);
		uBytesWritten = InBufferSize;
	}
	else
	{
		const uint32_t uMarginBytes = m_rightMargin;
		const uint32_t uFullRowCount = ((InBufferSize-uCurrentRowLeft) / uRowLen);
		const uint32_t uLaterRowFill = (InBufferSize-uCurrentRowLeft) - uFullRowCount * uRowLen;

		#ifdef _TRACE_ADD_MARGINS_
		logout("uMarginBytes=%d\n", uMarginBytes);
		logout("uFullRowCount=%d\n", uFullRowCount);
		logout("uLaterRowFill=%d\n", uLaterRowFill);
		#endif

		//Fill up row of the last run
		memcpy(&OutBuffer[0], &InBuffer[0], uCurrentRowLeft);
		#ifdef _TRACE_ADD_MARGINS_MEMCPY_
		logout("MEMCPY(0, 0, %d) s\n", uCurrentRowLeft);
		#endif

		uint32_t uiInIdx=uCurrentRowLeft;
		uint32_t uiOutIdx=uCurrentRowLeft + uMarginBytes;

		//Copy full rows
		for(uint32_t i=0; i<uFullRowCount; i++)
		{
			memcpy(&OutBuffer[uiOutIdx], &InBuffer[uiInIdx], uRowLen);
			#ifdef _TRACE_ADD_MARGINS_MEMCPY_
			logout("MEMCPY(%d, %d, %d) m\n", uiOutIdx, uiInIdx, uRowLen);
			#endif
			uiInIdx += uRowLen;
			uiOutIdx += uRowLen + uMarginBytes;
		}

		//Fill up next part of the current row
		if(0 < uLaterRowFill)
		{
			memcpy(&OutBuffer[uiOutIdx], &InBuffer[uiInIdx], uLaterRowFill);
			#ifdef _TRACE_ADD_MARGINS_MEMCPY_
			logout("MEMCPY(%d, %d, %d) e\n", uiOutIdx, uiInIdx, uLaterRowFill);
			#endif
			uiOutIdx += uLaterRowFill;
		}

		uBytesWritten = uiOutIdx;
	}

	#ifdef _TRACE_ADD_MARGINS_
	logout("uBytesWritten=%d\n", uBytesWritten);
	logout("-------------------\n");
	#endif

	#ifdef _DO_ERROR_CHECKS_
	if(OutBufferSize<uBytesWritten)
	{
		logout("ERROR: BUFFER OVERFLOW IN PushImageDataWithMargins()\n");
		exit(0);
	}
	#endif

	miTotalIndex += InBufferSize;
	return uBytesWritten;
}

/**
* Define margins for automatic margin adding in PushImageDataWithMargins()
*/
void CImageBuffer::SetMargins(const uint32_t RightMargin, const uint32_t TopMargin, const uint32_t BottomMargin, const uint32_t ImageWidth)
{
	m_rightMargin = RightMargin;
	m_topMargin = TopMargin;
	m_bottomMargin = BottomMargin;
	m_imageWidth = ImageWidth;
}

/**
* Push one decoded byte into the memory. Add boundaries at the top, bottom, left and right (to reduce an if statement
* in the later algorithm).
*/
inline void CImageBuffer::PushWithBounds(uint8_t *OutBuffer, uint32_t& OutIdx, const uint8_t Byte, const uint32_t OutBufferSize)
{
	//Add left and right margin beforehand - when this is a byte on the left edge of the picture
	if(0==(miTotalIndex%m_imageWidth))
	{
		const uint32_t iMarginBytes = m_rightMargin;
		#ifdef _DO_ERROR_CHECKS_
		if(iMarginBytes>OutBufferSize)
		{
			logout("ERROR:  BufferOverflowException(7)\n");
			exit(0);
		}
		#endif

		//memset(&OutBuffer[OutIdx], 0, iMarginBytes);
		OutIdx += iMarginBytes;
	}

	//Add picture data byte (RLE decompressed and inside margins)
	#ifdef _DO_ERROR_CHECKS_
	if(OutIdx>=OutBufferSize)
	{
		logout("ERROR:  BufferOverflowException(8)\n");
		exit(0);
	}
	#endif
	OutBuffer[OutIdx++] = Byte;
	miTotalIndex++;
}

/**
* Adds header margin (top margin and 1 row of left margin) to the decoder buffer
* Returns the amount of bytes written to OutBuffer
*/
uint32_t CImageBuffer::PushHeaderMargin(uint8_t *OutBuffer, const uint32_t OutBufferSize)
{

	//Note: Adds starting position - (1 left and 1 right margin) because in PushWithBounds() for the first byte (left+right margin) will be added.
	//So the first copied byte will be at the right position.
	const uint32_t iTotalWidth = m_imageWidth + m_rightMargin;
	const uint32_t iMarginBytes = m_topMargin*iTotalWidth/* - m_rightMargin*/;

	#ifdef _TRACE_ADD_MARGINS_
	logout("-------------------\n");
	logout("AddHeaderMargin:\n");
	logout("OutAddr=0x%x\n", &OutBuffer[0]);
	logout("iMarginBytes=%d\n", iMarginBytes);
	logout("-------------------\n");
	#endif

	#ifdef _DO_ERROR_CHECKS_
	if(OutBufferSize<iMarginBytes)
	{
		logout("ERROR:  BufferOverflowException(9)\n");
		exit(0);
	}
	#endif

	//memset(OutBuffer, 0, iMarginBytes);
	miTotalIndex=0;	//Position in the overall data buffer
	return iMarginBytes;
}

/**
* Adds footer margin (bottom margin and 1 row of right margin) to the decoder buffer
* Returns the amount of bytes written to OutBuffer
*/
uint32_t CImageBuffer::PushFooterMargin(uint8_t *OutBuffer, const uint32_t OutBufferSize)
{
	const uint32_t iTotalWidth = m_imageWidth + m_rightMargin;
	const uint32_t iMarginBytes = m_bottomMargin*iTotalWidth /*+ m_rightMargin*/;

	#ifdef _TRACE_ADD_MARGINS_
	logout("AddFooterMargin:\n");
	logout("OutAddr=0x%x\n", &OutBuffer[0]);
	logout("iMarginBytes=%d\n", iMarginBytes);
	logout("-------------------\n");
	#endif

	#ifdef _DO_ERROR_CHECKS_
	if(OutBufferSize<iMarginBytes)
	{
		logout("ERROR:  BufferOverflowException(10)\n");
		exit(0);
	}
	#endif

	return iMarginBytes;
}
