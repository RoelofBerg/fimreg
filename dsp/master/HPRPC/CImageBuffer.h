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

/**
* Buffer to receive image data into. This buffer is capable of adding (dirichlet zero condition) image margins while receiving a continuous pixelstream.
* The buffer assumes the target image memory to be zeroized (or intialized to another useful luminance) beforehand.
*
* ToDo: Enhance: Place images directly into the target memory place. At least for the template image T which needs no margins the memcoy in PushImageData seems an unnecessary overhead to me.
* (This code is still somewhat tied to the project's beginning time when the application ran over Ethernet.)
*/
class CImageBuffer
{
public:
	CImageBuffer();
	virtual ~CImageBuffer();

	uint32_t PushImageData(uint8_t *OutBuffer, const uint32_t OutBufferSize, const uint8_t *InBuffer, const uint32_t InBufferSize);
	uint32_t PushImageDataWithMargins(uint8_t *OutBuffer, const uint32_t OutBufferSize, const uint8_t *InBuffer, const uint32_t InBufferSize);
	uint32_t PushHeaderMargin(uint8_t *OutBuffer, const uint32_t OutBufferSize);
	uint32_t PushFooterMargin(uint8_t *OutBuffer, const uint32_t OutBufferSize);
	void SetMargins(const uint32_t RightMargin, const uint32_t TopMargin, const uint32_t BottomMargin, const uint32_t ImageWidth);

private:
	void PushWithBounds(uint8_t *OutBuffer, uint32_t& OutIdx, const uint8_t Byte, const uint32_t OutBufferSize);

	//statefulness
	uint32_t miTotalIndex;		//for auto added margins

	//for automatic margin adding
	uint32_t m_rightMargin;
	uint32_t m_topMargin;
	uint32_t m_bottomMargin;
	uint32_t m_imageWidth;
};
