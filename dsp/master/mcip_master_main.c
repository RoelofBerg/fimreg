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
#include "shared_mem_serial.h"
#include "logout.h"

#include "mcip_core.h"
#include "mcip_process.h"

/* BIOS6 include */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/family/c66/tci66xx/CpIntc.h>
#include <ti/ipc/Ipc.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/HeapBufMP.h>
#include <ti/ipc/MultiProc.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/cfg/global.h>

//RBE ToDo: Remove unused headers in the following block
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/IHeap.h>
#include <ti/ipc/Ipc.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/HeapBufMP.h>
#include <ti/ipc/MultiProc.h>
#include <ti/sysbios/BIOS.h>

#include <ti/csl/csl_bootcfgAux.h>

//rbe
#include <ti/sysbios/family/c64p/Exception.h>
#include <ti/sysbios/knl/Semaphore.h>

//----------------------------------------------------

/* Platform utilities include */
#include "ti/platform/platform.h"

/* Resource manager for QMSS, PA, CPPI */
#include "ti/platform/resource_mgr.h"

// When USE_OLD_SERVERS set to zero, server daemon is used
#define USE_OLD_SERVERS 0

/* Platform Information - we will read it form the Platform Library */
platform_info	gPlatformInfo;

Hwi_Params hwiParams;

static int number_of_cores = 0;

//RBE TODO: IS THE FOLLOWING STILL NECESSARY ???
#define GPIO_IN_DATA	0x02320020	/* for DSP number */
int get_DSP_id(void)
{
#ifdef  _DEF_DSPC_8681_
	return (((*(unsigned int*) GPIO_IN_DATA) & 0x6) >> 1);		/* GPIO 1~2 */
#else
	return (((*(unsigned int*) GPIO_IN_DATA) & 0xE) >> 1);
#endif
}

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))
#define IPCGR(x)            (0x02620240 + x*4)

int Init_QMSS_CPPI();
int Init_IPC();

/*************************************************************************
 *  @b EVM_init()
 * 
 *  @n
 * 		
 *  Initializes the platform hardware. This routine is configured to start in 
 * 	the evm.cfg configuration file. It is the first routine that BIOS 
 * 	calls and is executed before Main is called. If you are debugging within
 *  CCS the default option in your target configuration file may be to execute 
 *  all code up until Main as the image loads. To debug this you should disable
 *  that option. 
 *
 *  @param[in]  None
 * 
 *  @retval
 *      None
 ************************************************************************/
void EVM_init()
{
	platform_init_flags  	sFlags;
	platform_init_config 	sConfig;
	/* Status of the call to initialize the platform */
	int32_t pform_status;
    /* Platform Information - we will read it form the Platform Library */
    platform_info    sPlatformInfo;
    int core;

	/* 
	 * You can choose what to initialize on the platform by setting the following 
	 * flags. Things like the DDR, PLL, etc should have been set by the boot loader.
	*/
	memset( (void *) &sFlags,  0, sizeof(platform_init_flags));
	memset( (void *) &sConfig, 0, sizeof(platform_init_config));

	sFlags.pll  = 0;	/* PLLs for clocking  	*/
	sFlags.ddr  = 0;   	/* External memory 		*/
    sFlags.tcsl = 1;	/* Time stamp counter 	*/
    sFlags.phy  = 1;	/* Ethernet 			*/	//ToDo: Shouldn't this better be 0 ?
  	sFlags.ecc  = 0;	/* Memory ECC 			*/

    sConfig.pllm = 0;	/* Use libraries default clock divisor */

	pform_status = platform_init(&sFlags, &sConfig);

    /* If we initialized the platform okay */
    if (pform_status == Platform_EOK) {
        /* Get information about the platform so we can use it in various places */
        memset( (void *) &sPlatformInfo, 0, sizeof(platform_info));
        platform_get_info(&sPlatformInfo);
        number_of_cores = sPlatformInfo.cpu.core_count;
        MultiProc_setLocalId((Uint16) platform_get_coreid());
    } else {
        /* Initialization of the platform failed... die */
        logout("Platform failed to initialize. Error code %d \n", pform_status);
		while (1) {
			(void) platform_led(1, PLATFORM_LED_ON, PLATFORM_USER_LED_CLASS);
			(void) platform_delay(50000);
			(void) platform_led(1, PLATFORM_LED_OFF, PLATFORM_USER_LED_CLASS);
			(void) platform_delay(50000);
		}
    }

    /* Unlock the chip registers */
    CSL_BootCfgUnlockKicker();

    /* wake up the other core to run slave part */
    for (core = 1; core < CORE_AMOUNT; core++)
    {
        /* IPC interrupt other cores */
        DEVICE_REG32_W(IPCGR(core), 1);
        platform_delay(1000);
    }
}


#define USE_MSI_INT
void interrupt_register()
{
	hprpc_irq_handler_init();

	/* Clear MSI0 interrupt */
	*((unsigned int *)MSI0_IRQ_STATUS) = 0x1;
	*((unsigned int *)TI667X_IRQ_EOI) = 0x4;		/* end of MSI0, event number=4 */

	Hwi_Params_init(&hwiParams);

	/* set the argument you want passed to your ISR function */
	hwiParams.arg = (Int32)&hwiParams;

	/* set the event id of the peripheral assigned to this interrupt */
	hwiParams.eventId = 17;	/* for MSI interrupt */

	/* Enable the Hwi */
	hwiParams.enableInt = TRUE;

	/* don't allow this interrupt to nest itself */
	hwiParams.maskSetting = Hwi_MaskingOption_SELF;

	/* Configure interrupt 4 to invoke the hprpc_irq_handler. */
	/* Automatically enables interrupt 4 by default */
	/* set params.enableInt = FALSE if you want to control */
	/* when the interrupt is enabled using Hwi_enableInterrupt() */
	Hwi_create(INTC_VECTID, hprpc_irq_handler, &hwiParams, NULL);
}

//---------------------------------------------------------------------
// Main Entry Point for the master core
//---------------------------------------------------------------------
int master_main()
{
	//ToDo: Necessary ??? (See below ...)
	Init_QMSS_CPPI();

	Init_IPC();

	//Register PCIe IRQ
	interrupt_register();

    // Initialization finished
    logout("Ready to receive PCIe commands ...\n\n");

    // Process HPRPC requests coming in over PCIe (never returns)
    hprpc_serve_irqs();

    return 0;
}


//---------------------------------------------------------------------
// Main Entry Point
//---------------------------------------------------------------------
int main()
{
	#ifndef _PCIE_VUART_DISABLED_
	//virtual uart
	serial_init();
	#endif
	logout("\n\n==== DSPREG PCIe EDITION ====\n");
	logout("You're listening on core 0.\n");

    /*
     *  Ipc_start() calls Ipc_attach() to synchronize all remote processors
     *  because 'Ipc.procSync' is set to 'Ipc.ProcSync_ALL' in *.cfg
     */
    Ipc_start();

    /* Start the BIOS 6 Scheduler */
	BIOS_start ();

	return 0;
}


int Init_IPC()
{
    Int              status;
    HeapBufMP_Handle heapHandle;
    HeapBufMP_Params heapBufParams;

    /* Create the heap that will be used to allocate messages. */
    HeapBufMP_Params_init(&heapBufParams);
    heapBufParams.regionId       = 0;
    heapBufParams.name           = IMAGE_PROCESSING_HEAP_NAME;
    heapBufParams.numBlocks      = number_of_cores;
    heapBufParams.blockSize      = sizeof(process_message_t);
    heapHandle = HeapBufMP_create(&heapBufParams);
    if (heapHandle == NULL) {
        logout("Main: HeapBufMP_create failed\n" );
        goto ipc_exit;
    }

    /* Register this heap with MessageQ */
    status = MessageQ_registerHeap((IHeap_Handle)heapHandle, IMAGE_PROCESSING_HEAPID);
    if(status != MessageQ_S_SUCCESS) {
        logout("Main: MessageQ_registerHeap failed\n" );
        goto ipc_exit;
    }

    if (mc_process_init(number_of_cores)) {
        logout("mc_process_init returns error\n");
        goto ipc_exit;
    }

ipc_exit:
	return(0);
}

//
// Initialize QMSS and CPPI
//
//RBE TODO: DO WE NEED QMSS AND CPPI ???
//If not also delete resourcemgr.c and in client.cfg the Program.sectMap-entries named .resmgr*
//
int Init_QMSS_CPPI()
{
    int				  i;
    QMSS_CFG_T      qmss_cfg;
    CPPI_CFG_T      cppi_cfg;

	/* Get information about the platform so we can use it in various places */
	memset( (void *) &gPlatformInfo, 0, sizeof(platform_info));
	(void) platform_get_info(&gPlatformInfo);

	(void) platform_uart_init();
	(void) platform_uart_set_baudrate(115200);
	(void) platform_write_configure(PLATFORM_WRITE_ALL);

	/* Clear the state of the User LEDs to OFF */
	for (i=0; i < gPlatformInfo.led[PLATFORM_USER_LED_CLASS].count; i++) {
		(void) platform_led(i, PLATFORM_LED_OFF, PLATFORM_USER_LED_CLASS);
	}

    /* Initialize the components required to run this application:
     *  (1) QMSS
     *  (2) CPPI
     *  (3) Packet Accelerator
     */
    /* Initialize QMSS */
    if (platform_get_coreid() == 0)
    {
        qmss_cfg.master_core        = 1;
    }
    else
    {
        qmss_cfg.master_core        = 0;
    }
    qmss_cfg.max_num_desc       = MAX_NUM_DESC;
    qmss_cfg.desc_size          = MAX_DESC_SIZE;
    qmss_cfg.mem_region         = Qmss_MemRegion_MEMORY_REGION0;
    if (res_mgr_init_qmss (&qmss_cfg) != 0)
    {
        logout ("Failed to initialize the QMSS subsystem \n");
        goto main_exit;
    }
    else
    {
    	logout ("QMSS successfully initialized \n");
    }

    /* Initialize CPPI */

    if (platform_get_coreid() == 0)
    {
        cppi_cfg.master_core        = 1;
    }
    else
    {
        cppi_cfg.master_core        = 0;
    }
    cppi_cfg.dma_num            = Cppi_CpDma_PASS_CPDMA;
    cppi_cfg.num_tx_queues      = NUM_PA_TX_QUEUES;
    cppi_cfg.num_rx_channels    = NUM_PA_RX_CHANNELS;
    if (res_mgr_init_cppi (&cppi_cfg) != 0)
    {
        logout ("Failed to initialize CPPI subsystem \n");
        goto main_exit;
    }
    else
    {
    	logout ("CPPI successfully initialized \n");
    }

main_exit:
    return(0);
}

void myInternalHook(void)
{
      Exception_Status status;
      Ptr a31, a30;
      Ptr b31, b30;

     Exception_getLastStatus(&status);

     logout("myInternalHook:\n");
     logout("  efr=0x%x\n", status.efr);
     logout("  nrp=0x%x\n", status.nrp);
     logout("  ntsr=0x%x\n", status.ntsr);
     logout("  ierr=0x%x\n", status.ierr);
     logout("  excContext=0x%x\n", status.excContext);
      a31 = status.excContext->A31;
      a30 = status.excContext->A30;
      b31 = status.excContext->B31;
      b30 = status.excContext->B30;
      logout("  A31_A30=0x%08x%08x\n",
                    (UInt)(a31),
                    (UInt)(a30));
      logout("  B31_B30=0x%08x%08x\n",
                    (UInt)(b31),
                    (UInt)(b30));
}

void myExceptionHook(void)
{
	myInternalHook();
}
