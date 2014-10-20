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

#include "fimreg_common.h"

#include "CHPRPCRequest.h"
class CHPRPCConnection;

#pragma pack(1)
/** Representation in the DMA data stream */
struct HPRPC_HDR_Request_CalcSSDJacHess
{
	uint16_t NumberOfCores;
	uint16_t Level;
	t_reg_real w[3];
};
#pragma pack()

//Forward declaration
class CHPRPCRequest;

/**
* Class for parsing and generation of HPRPC commands 'CalcSSDJacHess' and 'CalcSSD'.
* As both commands have the same payload (w) and only differ in the OpCode we can use
* one single class for both commands. A ctor parameter determines wether only the SSD
* or wether also the Jacobian and the Hessian shall be calculated.
*/
class CHPRPCRequestCalcSSDJacHess : public CHPRPCRequest
{
public:
	enum CalcSSDMode
	{
		CALC_SSD_JAC_HESS,
		CALC_SSD_ONLY
	};

	CHPRPCRequestCalcSSDJacHess(CHPRPCConnection& HPRPCConnection, CalcSSDMode CalculationMode);
	virtual ~CHPRPCRequestCalcSSDJacHess(void);

	void AssignValuesToBeSent(const t_reg_real w[3], const uint16_t NumberOfCores, const uint16_t Level);

	HPRPC_HDR_Request_CalcSSDJacHess* GetPayloadHeader();

private:
	CalcSSDMode meCalculationMode;
};
