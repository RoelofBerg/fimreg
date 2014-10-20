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

#include <stdio.h>

#include "pcie_irq_handler.h"
#include "CHPRPCServer.h"

//ToDo: Remove unnecessary headers

/* BIOS6 include */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/family/c66/tci66xx/CpIntc.h>

#include <xdc/cfg/global.h>	//constants from .cfg file like BUFSIZE_HPRPC_DMA


/* Platform utilities include */
#include "ti/platform/platform.h"

/* Resource manager for QMSS, PA, CPPI */
#include "ti/platform/resource_mgr.h"

#include <ti/sysbios/knl/Semaphore.h>

#include "logout.h"

// Reserve buffer dor DMA communication which is tied to a certain address space in the SYSBIOS .cfg file
#pragma DATA_SECTION (g_hprpc_dma_memory, ".hprpcDMA");
Uint8 g_hprpc_dma_memory[BUFSIZE_HPRPC_DMA];

/**
 * Initialize stuff needed in hprpc_irq_handler()
 */
void hprpc_irq_handler_init()
{
	memset((void *)g_hprpc_dma_memory, 0, BUFSIZE_HPRPC_DMA);
}

/**
 * PCIe interrupt signaling incoming data
 */
void hprpc_irq_handler (UArg Params)
{
	Hwi_Params *hwiParams = (Hwi_Params *)Params;

	//logout("   IRQ eventId=%u\n", hwiParams->eventId);

	if(CpIntc_getEventId(HOST_INT_ID) == hwiParams->eventId)
	{
		/* Clear legacy intB */
		*((unsigned int *)LEGACY_B_IRQ_STATUS) = 0x1;
		*((unsigned int *)TI667X_IRQ_EOI) = 0x1;
	}
	else if(hwiParams->eventId == 17)
	{
		/* Clear MSI0 interrupt */
		*((unsigned int *)MSI0_IRQ_STATUS) = 0x1;
		*((unsigned int *)TI667X_IRQ_EOI) = 0x4;		/* end of MSI0, event number=4 */
	}
	else
		return;

	#ifdef _TRACE_HPRPC_VERBOSE_
	logout("   IRQ -> SemaphorePost()\n");
	#endif

	//Wake up master_main() to call the method hprpc_irq_process() below in the context of the master_main() task.
	Semaphore_post(pcie_semaphore);
}

/**
 * Blocks for IRQs (HPRPC requests) to arrive and calls ProcessIncomingDataFrame() for processing the HPRPC requests.
 * Executed in the context of the master_main() task
 */
void hprpc_serve_irqs()
{
    while(1)
    {
		#ifdef _TRACE_HPRPC_VERBOSE_
    	logout("   MasterMain going to sleep -> SemaphorePend()\n");
		#endif

    	//Wait until we receive a PCIe hardware interrupt in hprpc_irq_handler()
    	Semaphore_pend(pcie_semaphore, BIOS_WAIT_FOREVER);

		#ifdef _TRACE_HPRPC_VERBOSE_
		logout("   MasterMain woken up -> hprpc_irq_process()\n");
		#endif

    	//Process the received PCIe data (a HPRPC command)
		ProcessIncomingDataFrame(g_hprpc_dma_memory, BUFSIZE_HPRPC_DMA, g_hprpc_dma_memory, BUFSIZE_HPRPC_DMA);
    }
}

/**
 * Generate interrupt to host telling that there is a HPRPC response.
 * Executed in the context of the master_main() task, will be called
 * while master_main is processing ProcessIncomingDataFrame().
 */
void hprpc_send_irq_to_host()
{
	*((unsigned int *)TI667X_EP_IRQ_SET) = 0x1;
}
