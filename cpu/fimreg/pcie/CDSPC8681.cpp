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

#include "CDSPC8681.h"

#include <setupapi.h>
#include <initguid.h>
#include "Device.h"
#include "Common.h"
#include "Ioctls.h"
#include "guid.h"
#include "PowerSleepController.h"
#include "BootConfig.h"
#include "Ti667xDevice.h"
#include "Dspc868xCard.h"
#include "Dspc8681Card.h"
#include "Dspc8682Card.h"
extern PCHAR MapSystemErrorCode(DWORD Code);

CDSPC8681::CDSPC8681(void)
	: m_Dspc8681Card(NULL)
{
}


CDSPC8681::~CDSPC8681(void)
{
	if(m_Dspc8681Card != NULL)
	{
		printf("Dtor of CDSPC8681 called without proper CDSPC8681::Shutdown(). Enforcing CDSPC8681::Shutdown().\n");
		Shutdown();
	}
}

bool CDSPC8681::Initialize(void)
{
	DWORD dwCardIndex = 0;
	DWORD dwError = ERROR_SUCCESS;
	m_Dspc8681Card = new Dspc8681Card(dwCardIndex, dwError);

	if (m_Dspc8681Card == NULL)
	{
		printf("can't instantiate object, bye.\n");
		return false;
	}

	if (dwError != ERROR_SUCCESS)
	{
		printf("object initialization failed. Error: %s.\n", MapSystemErrorCode(dwError));
		delete m_Dspc8681Card;
		m_Dspc8681Card = NULL;
		return false;
	}

	dwError = m_Dspc8681Card->Open();
	//Sleep(100);

	if (dwError != ERROR_SUCCESS)
	{
		printf("Unable to open device. Error: %s\n", MapSystemErrorCode(dwError));
		return false;
	}

	return true;

}

void CDSPC8681::Shutdown(void)
{
	if(m_Dspc8681Card == NULL)
		return;

	
	#ifdef _TRACE_DMA_
	printf("Close Dspc8681 PCIe device.\n");
	#endif

	DWORD dwError = m_Dspc8681Card->Close();
	if(dwError != ERROR_SUCCESS)
	{
		printf("Unable to close device. Error: %s\n", MapSystemErrorCode(dwError));
	}

	//Sleep(1000);

	delete m_Dspc8681Card;
	m_Dspc8681Card = NULL;
}

/**
* Return direct access to the Advantech driver layer
*/
Dspc8681Card* CDSPC8681::getDspc8681CardDriver()
{
	return m_Dspc8681Card;
}