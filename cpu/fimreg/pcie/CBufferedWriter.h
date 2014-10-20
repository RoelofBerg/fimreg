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

#include "ISendableBufferProvider.h"

/**
* Construct by passing an object of the interface ISendableBufferProvider. Then pass the data to be sent to Enqueue() (like to a stream).
* When finished call Send() in order to start sending the buffers.
*
* This is a version that automatically adds a 4 byte length-descriptor to the start and an end-token to the end. (We need this on the DSP to detect the end of the DMA transfer).
*
* !!!!!
* ToDo: resolve circle references: Add a CBufferProvider passed to here in the ctor. Remove the reference to ISendableBufferProvider. And instead of Send() use PrepareForSending() that returns the length. Then in CHPRPCCommand don't call BufferedWriter.Send() but PCIeConn.Send() which then calls PrepareForSending().
* !!!!!
*/
class CBufferedWriter
{
public:
	CBufferedWriter(ISendableBufferProvider* SendableBufferProvider);
	virtual ~CBufferedWriter(void);

	//Interface ICanEnqueueAndSendBuffers
	void Enqueue(const uint8_t* buffer, const uint32_t size);
	void Send();

private:
	//Buffer processing
	uint8_t* m_KernelBufCursor;
	uint32_t m_KernelBufRemainingBytes;
	ISendableBufferProvider* m_pSendableBufferProvider;

	//Processing of the length prefix
	uint32_t* m_pLengthPrefix;
	uint32_t m_totalLength;			//Total data length including the end-token, excluding the beginning length-marker

	void EnqueueEndToken();
	void EnqueueDummyStartToken();
	void WriteTotalLengthToStartToken();
};


