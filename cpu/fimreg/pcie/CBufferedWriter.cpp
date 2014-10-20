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

#include "CBufferedWriter.h"

CBufferedWriter::CBufferedWriter(ISendableBufferProvider* SendableBufferProvider)
: m_KernelBufCursor(NULL),
  m_KernelBufRemainingBytes(0),
  m_pSendableBufferProvider(SendableBufferProvider),
  m_pLengthPrefix(NULL),
  m_totalLength(0)
{
	EnqueueDummyStartToken();
}

CBufferedWriter::~CBufferedWriter(void)
{
}

/**
* Add data to be sent. (The data will be copied into the send buffers and it is allowed to modify it after this call.)
*/
void CBufferedWriter::Enqueue(const uint8_t* buffer, const uint32_t size)
{
	m_totalLength += size;
	uint32_t remainingSize = size;
	
	//Do we need another buffer ? If yes, flush everything that's possible and allocate a new one.
	while(m_KernelBufRemainingBytes < remainingSize)
	{
		if(0 < m_KernelBufRemainingBytes)
		{
			memcpy(m_KernelBufCursor, buffer, m_KernelBufRemainingBytes);
			buffer += m_KernelBufRemainingBytes;
			remainingSize -= m_KernelBufRemainingBytes;
		}

		bool bSuccess = m_pSendableBufferProvider->GetNextBuffer(m_KernelBufCursor, m_KernelBufRemainingBytes);
		if (false == bSuccess)
		{
			printf("Error: BufferedWriter was unable to obtain more transmission buffers.\n");
			throw OutOfMemoryException();
		}
	}

	//Flush the rest (or everything if the above while loop wasn't entered) into the current buffer
	memcpy(m_KernelBufCursor, buffer, remainingSize);
	m_KernelBufCursor += remainingSize;
	m_KernelBufRemainingBytes -= remainingSize;
}

/**
* Send all data enqueued with the Enqueue() command to the remote peer
*/
void CBufferedWriter::Send()
{
	//Add correct start and end tokens (our proprietary PCIe-layer protocol)
	EnqueueEndToken();
	WriteTotalLengthToStartToken();

	//Send everything (nonblocking)
	m_pSendableBufferProvider->Send(m_totalLength + 4);	//Send data (+4 = start length descriptor)
}

/**
* The DSP performs a busy waiting for an end token (which is the best tradeoff between low-main-cpu-usage and responsiveness). We add it to the stream automatically during the Send() procedure.
*/
void CBufferedWriter::EnqueueEndToken()
{
	uint32_t endToken = 0xdeadbeef;
	Enqueue((uint8_t*)(&endToken), sizeof(endToken));
}

/**
* In CPU-Transfer-Mode the API will add the length descriptor for us. In DMA mode we also use this behavior (it is,
* however, redundant to the HPRPC protocol ... maybe it'd be the best to change the protocol ...)
*
* In DMA mode we prefix the transfer with the transfer length (and set the MSB to signalize DMA mode)
* Here we add only a placeholder, later we will overwrite it during Send() (see OverwriteStartToken()).
*/
void CBufferedWriter::EnqueueDummyStartToken()
{

	uint32_t lengthPlaceholder = 0;
	uint32_t sizeofPlaceholder = sizeof(lengthPlaceholder);
	Enqueue((uint8_t*)(&lengthPlaceholder), sizeofPlaceholder);
	m_pLengthPrefix = (uint32_t*)(m_KernelBufCursor - sizeofPlaceholder);	//remember this place, we will write the total length to it when we finally send the buffers
}

/**
* Add the total transfer length (excluding this start-token itself but including the end-token) to the very beginning of the first buffer.
*/
void CBufferedWriter::WriteTotalLengthToStartToken()
{
	m_pLengthPrefix[0] = (0x80000000) | (m_totalLength-4);	//Also set the MSB in order to signalize that this is a DMA (and no CPU-based) transfer. (Enter length wo. the first length token itself)
	m_pLengthPrefix[2] = m_totalLength-8; //Length of the HPRPC payload (this is the overall length without the PCIe related start-descriptor and the PCIe related end token). 
}

