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
#include "CHPRPCResponse.h"

//Forward declaration
class CHPRPCResponse;

/** Representation in the data stream */
struct HPRPC_HDR_Response_StoreImg
{
	uint32_t CyclesForAddMargins;	//@see CHPRPCResponseStoreImg, needs _TRACE_PYRAMID_TIME_ compile switch on the DSP
	uint32_t CyclesForPyramid;		//@see CHPRPCResponseStoreImg, needs _TRACE_PYRAMID_TIME_ compile switch on the DSP
};

/**
* Response to the StoreImg HPRPC request. It contains only debug information (needed DSP cycles for the margin/pyramid generation)
* The compile switch _TRACE_PYRAMID_TIME_ must be activated on the DSP (which unfortunately could affect the overall timing in a negative way).
*/
class CHPRPCResponseStoreImg : public CHPRPCResponse
{
public:
	CHPRPCResponseStoreImg();
	virtual ~CHPRPCResponseStoreImg(void);

	void AssignValuesToBeSent(const uint16_t CallIndex, const uint32_t TimeForAddMargins, const uint32_t TimeForPyramid);
	void AssignErrorToBeSent(const uint16_t CallIndex, const uint16_t ErrorCode, const string& ErrorMessage);
	HPRPC_HDR_Response_StoreImg* GetPayloadHeader();

private:
};
