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

#include <c6x.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/IHeap.h>
#include <ti/ipc/Ipc.h>
#include <ti/ipc/MessageQ.h>
#include <ti/ipc/HeapBufMP.h>
#include <ti/ipc/MultiProc.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/cfg/global.h>

#include "mcip_core.h"
#include "logout.h"

#include "cache_handling.h"

#include "ssd_on_core.h"
#include "jacobian_on_core.h"
#include "shrinkImage_on_core.h"

void exec_ssd(process_message_t* p_msg);
void exec_ssdJacHess(process_message_t* p_msg);
void exec_shrinkImage(process_message_t* p_msg);
void CacheInvalTotalMemory(process_message_t* p_msg);
void CacheInvalImageData(process_message_t* p_msg);

void slave_main(void)
{
    process_message_t * p_msg = 0;    
    MessageQ_Handle  h_receive_queue = 0;
    MessageQ_QueueId reply_queue_id = 0;
    HeapBufMP_Handle heapHandle;
    Int status;
    char receive_queue_name[16];

    GET_SLAVE_QUEUE_NAME(receive_queue_name, DNUM);

    /* Open the heap created by the other processor. Loop until opened. */    
    do {        
        status = HeapBufMP_open(IMAGE_PROCESSING_HEAP_NAME, &heapHandle);
        if (status < 0) { 
            Task_sleep(1);
        }
    } while (status < 0);

    /* Register this heap with MessageQ */    
    MessageQ_registerHeap((IHeap_Handle)heapHandle, IMAGE_PROCESSING_HEAPID);
    
    /* Create the local message queue */
    h_receive_queue = MessageQ_create(receive_queue_name, NULL);    
    if (h_receive_queue == NULL) {
        logout("MessageQ_create failed\n" );
		goto close_n_exit;
    }

	for (;;) {
 
		if (MessageQ_get(h_receive_queue, (MessageQ_Msg *)&p_msg, MessageQ_FOREVER) < 0) {
		    logout("%s: This should not happen since timeout is forever\n", receive_queue_name);
		    goto close_n_exit;
		}

        reply_queue_id = MessageQ_getReplyQueue(p_msg);
        if (reply_queue_id == MessageQ_INVALIDMESSAGEQ) {
            logout("receive_queue_name: Ignoring the message as reply queue is not set.\n", receive_queue_name);
            continue;
        }

        //Execute calculation
		#ifdef _TRACE_MC_
        logout("[core_%u] Execute process (processing_type=%u)\n", p_msg->core_id, p_msg->info.processing_type); //trace
		#endif

        switch(p_msg->info.processing_type)
        {
        	case pt_ssd:
        		//Call calculation code
        		exec_ssd(p_msg);
        		break;

        	case pt_ssdJacHess:
        		//Call calculation code
        		exec_ssdJacHess(p_msg);
        		break;

        	case pt_cacheinval:
        		CacheInvalTotalMemory(p_msg);
        		break;

        	case pt_shrink:
        		//Call image shrink code
        		exec_shrinkImage(p_msg);
        		break;

        	default:
        		logout("Invalid IPC processing type: %u", p_msg->info.processing_type);
        }

        /* send the message to the remote processor */
		#ifdef _TRACE_MC_
		logout("[core_%u] Putting slave response to the MessageQ, then going idle again ...\n", p_msg->core_id);
		#endif
        if (MessageQ_put(reply_queue_id, (MessageQ_Msg)p_msg) < 0) {
            logout("%s: MessageQ_put had a failure error\n", receive_queue_name);
        }

	}

close_n_exit:
    if(h_receive_queue) MessageQ_delete(&h_receive_queue);
}

void exec_ssd(process_message_t* p_msg)
{
	const uint8_T *Tvec = p_msg->info.Tvec;
	const uint8_T *Rvec = p_msg->info.Rvec;
	#ifdef _TRACE_OUTPUT_
	logout("TOffset=%d, ROffset=%d\n", p_msg->info.TOffset, p_msg->info.ROffset);
	#endif

	//Cache invalidate image data if necessary
	CacheInvalImageData(p_msg);

	//Execute calculation on the assigned DSP core
	real32_T result = ssd_on_core(p_msg->info.w, p_msg->info.BoundBox, p_msg->info.MarginAddon, p_msg->info.DSPRange, Tvec, p_msg->info.TOffset, Rvec, p_msg->info.ROffset,
		p_msg->info.d, p_msg->info.i_from, p_msg->info.i_to);

	#ifdef _TRACE_MC_
	logout("[core_%u] SSD = %f\n", p_msg->core_id, (double)result);
	#endif

    //Write back calculation result to message answer
    p_msg->info.out_SSD = result;
}

void exec_ssdJacHess(process_message_t* p_msg)
{
	real32_T SSD;
	real32_T JD[3];
	real32_T JD2[9];
	const uint8_T *Tvec = p_msg->info.Tvec;
	const uint8_T *Rvec = p_msg->info.Rvec;
	#ifdef _TRACE_OUTPUT_
	logout("TOffset=%d, ROffset=%d\n", p_msg->info.TOffset, p_msg->info.ROffset);
	#endif

	//Cache invalidate image data if necessary
	CacheInvalImageData(p_msg);

	//Execute calculation on the assigned DSP core
	jacobian_on_core(p_msg->info.w, p_msg->info.BoundBox, p_msg->info.MarginAddon, p_msg->info.DSPRange, Tvec, p_msg->info.TOffset, Rvec, p_msg->info.ROffset,
		p_msg->info.d, &SSD, JD, JD2, p_msg->info.i_from, p_msg->info.i_to);

	#ifdef _TRACE_MC_
	logout("[core_%u] SSD = %f, JD = [%f %f %f], JD2 = [%f ... %f ... %f]\n", p_msg->core_id, (double)SSD, (double)JD[0], (double)JD[1], (double)JD[2], (double)JD2[0], (double)JD2[4], (double)JD2[8]);
	#endif

	//Write back calculation result to message answer (we don't pass the pointers directly and copy instead to ensure pointers are on fast L2 stack memory).
	p_msg->info.out_SSD = SSD;
	memcpy(p_msg->info.out_JD, JD, sizeof(real32_T) * 3);
	memcpy(p_msg->info.out_JD2, JD2, sizeof(real32_T) * 9);
}

void exec_shrinkImage(process_message_t* p_msg)
{
	//Execute calculation on the assigned DSP core (function will invalidate and write back cache data internally).
	shrinkImage_on_core(p_msg->info.Shr_Img, p_msg->info.Shr_SubArea, p_msg->info.Shr_yStart, p_msg->info.Shr_yEnd, p_msg->info.Shr_yEvenOdd, p_msg->info.Shr_yHeight,
			p_msg->info.Shr_xWidth, p_msg->info.Shr_xWidthSmall, p_msg->info.Shr_ImgSmall);

	#ifdef _TRACE_MC_
	logout("[core_%u] Finished shrinking image part for y from %d to %d.\n", p_msg->core_id, p_msg->info.Shr_yStart, p_msg->info.Shr_yEnd);
	#endif
}

/**
 * Cache invalidate the total image memory (happens after the main core zeroizes all memory (at boot time and after/between the calculations)
 */
void CacheInvalTotalMemory(process_message_t* p_msg)
{
	if (0 != MultiProc_self()) {
		#ifdef _TRACE_MC_
			logout("[core_%u] CACHE INVALIDATE ZEROIZRD DATA ON SLAVE CORE (T %u bytes, R %u bytes)\n", p_msg->core_id, p_msg->info.Tsize, p_msg->info.Rsize);
		#endif
		Cache_inv((xdc_Ptr*) p_msg->info.Tvec, p_msg->info.Tsize , Cache_Type_ALL, CACHE_BLOCKING);
	} else {
		#ifdef _TRACE_MC_
			//The following shouldn't happen ...
			logout("[core_%u] CACHE INVALIDATE ZEROIZRD DATA SKIPPED ON MAIN CORE (T %u bytes, R %u bytes)\n", p_msg->core_id, p_msg->info.Tsize, p_msg->info.Rsize);
		#endif
	}

}

/**
 * Cache invalidate image data if necessary
 */
void CacheInvalImageData(process_message_t* p_msg)
{
	if(0 != p_msg->info.NewImageDataArrived)
	{
		if (0 != MultiProc_self()) {
			#ifdef _TRACE_MC_
				logout("[core_%u] CACHE INVALIDATE IMAGE DATA ON SLAVE CORE (T %u bytes, R %u bytes)\n", p_msg->core_id, p_msg->info.Tsize, p_msg->info.Rsize);
			#endif
			Cache_inv((xdc_Ptr*) p_msg->info.Tvec, p_msg->info.Tsize , Cache_Type_ALL, CACHE_BLOCKING);
			Cache_inv((xdc_Ptr*) p_msg->info.Rvec, p_msg->info.Rsize , Cache_Type_ALL, CACHE_BLOCKING);
		} else {
			#ifdef _TRACE_MC_
				logout("[core_%u] CACHE INVALIDATE SKIPPED ON MAIN CORE (T %u bytes, R %u bytes)\n", p_msg->core_id, p_msg->info.Tsize, p_msg->info.Rsize);
			#endif
		}
	}
}
