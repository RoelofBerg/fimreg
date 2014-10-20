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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/Types.h>
#include <xdc/runtime/Timestamp.h>
#include <ti/sysbios/hal/Cache.h>

#include <xdc/cfg/global.h>

#include <xdc/runtime/Log.h>

#include "mcip_process.h"

#include "mcip_core.h"

#include "logout.h"
#ifdef _NO_IPC_TEST_
#include "ssd_on_core.h"
#include "jacobian_on_core.h"
#include "shrinkImage_on_core.h"
#endif

#if 0
/* w should be power of 2 */
#define ROUNDUP(n,w) (((n) + (w) - 1) & ~((w) - 1))
#define MAX_CACHE_LINE (128)
#endif

#define MAX_SLICES (10) /* This should be more than # of cores in device */
#define DEFAULT_SLICE_OVERLAP_SIZE 2

#define MASTER_QUEUE_NAME "master_queue"
static char slave_queue_name[MAX_SLICES][16];

static process_message_t ** p_queue_msg = 0;
static int max_core = 0;

//Marker if new image data is available (then all cores must invalidate the image data in the cache)
extern uint32_t g_NewImageDataArrived;

void prepare_ipc_messages_for_all_cores(const processing_type_e ProcessingType, const real32_T w[3], const uint32_T BoundBox[4], const uint32_T MarginAddon[3], const real32_T
             DSPRange[4], const emxArray_uint8_T *Tvec, const uint32_T TOffset, const emxArray_uint8_T *Rvec, const uint32_T ROffset,
             const uint32_T d, const uint32_T number_of_cores);

void prepare_ipc_message(const processing_type_e ProcessingType, const real32_T w[3], const uint32_T BoundBox[4], const uint32_T MarginAddon[3], const real32_T
             DSPRange[4], const emxArray_uint8_T *Tvec, const uint32_T TOffset, const emxArray_uint8_T *Rvec, const uint32_T ROffset,
             const uint32_T d, const uint32_T CoreNo, const uint32_T i_from, const uint32_T i_to);

void send_to_cores(const processing_type_e ProcessingType, const uint32_T number_of_cores, real32_T *SSD, real32_T JD[3], real32_T JD2[9]);

uint32_T get_core_id(const uint32_T CoreNo);

int init_message_q();
void openSlaveQueues(const uint32_T number_of_cores);

int mc_process_init (int number_of_cores)
{
    int i;

    p_queue_msg = (process_message_t **) calloc(number_of_cores, sizeof(process_message_t *));
    if (!p_queue_msg)
    {
        logout("alloc_queue_message: Can't allocate memory for queue message\n");
        return -1;
    }

    for (i = 0; i < number_of_cores; i++)
    {
        p_queue_msg[i] =  (process_message_t *) MessageQ_alloc(IMAGE_PROCESSING_HEAPID, sizeof(process_message_t));
        if (!p_queue_msg[i]) {
            logout("alloc_queue_message: Can't allocate memory for queue message %d\n", i);
            return -1;            
        }
    }

    max_core = number_of_cores;

    memset(slave_queue_name, 0, MAX_SLICES*16);
    for (i = 0; i < MAX_SLICES; i++)
    {
        GET_SLAVE_QUEUE_NAME(slave_queue_name[i], i);
    }

	//rbe: this was once below and executed each time (I hope it is faster when this is done here)
	int iRetVal = init_message_q();
	if(0 == iRetVal)
	{
		openSlaveQueues(max_core);
	}
	return iRetVal;
}

void jacobian(const real32_T w[3], const uint32_T BoundBox[4], const uint32_T MarginAddon[3], const real32_T DSPRange[4], const emxArray_uint8_T *Tvec, const uint32_T TOffset,
			  const emxArray_uint8_T *Rvec, const uint32_T ROffset, const uint32_T d, real32_T *SSD, real32_T JD[3], real32_T JD2[9],
			  const uint32_T number_of_cores)

{
#ifdef _NO_IPC_TEST_
   	const uint32_T i_range = (int32_T)(DSPRange[3]- DSPRange[2] + 1.0f);
	jacobian_on_core(w, BoundBox, MarginAddon, DSPRange, Tvec, TOffset, Rvec, ROffset, d, SSD, JD, JD2, 0, i_range);
#else
	prepare_ipc_messages_for_all_cores(pt_ssdJacHess, w, BoundBox, MarginAddon, DSPRange, Tvec, TOffset, Rvec, ROffset, d, number_of_cores);
	send_to_cores(pt_ssdJacHess, number_of_cores, SSD, JD, JD2);
#endif
}

real32_T ssd(const real32_T w[3], const uint32_T BoundBox[4], const uint32_T MarginAddon[3], const real32_T
             DSPRange[4], const emxArray_uint8_T *Tvec, const uint32_T TOffset, const emxArray_uint8_T *Rvec,
             const uint32_T ROffset, const uint32_T d, const uint32_T number_of_cores)
{
	real32_T SSD = 0;

#ifdef _NO_IPC_TEST_
   	const uint32_T i_range = (int32_T)(DSPRange[3]- DSPRange[2] + 1.0f);
   	SSD = ssd_on_core(w, BoundBox, MarginAddon, DSPRange, Tvec, TOffset, Rvec, ROffset, d, 0, i_range);
#else
	prepare_ipc_messages_for_all_cores(pt_ssd, w, BoundBox, MarginAddon, DSPRange, Tvec, TOffset, Rvec, ROffset, d, number_of_cores);
	send_to_cores(pt_ssd, number_of_cores, &SSD, NULL, NULL);
#endif

	return SSD;
}

/**
 * Cachec invalidate zeroized memory on all cores (happens between the calculations / after a calculation result and at boot time)
 */
void cacheinval_on_core(const uint8_T *TBuf, const uint32_T TSize, const uint32_T number_of_cores)
{
#ifndef _NO_IPC_TEST_

	int32_t i=0;

	//To see which entries have to be placed to which parameter look at the union declarations in struct processing_info.
	//Very ugly, I know. But this software is only a scientific proof of concept and it's near its end, hence this kind of code can be written now during the last days ...

	process_message_t * p_msg = 0;

    for (i = CORE_AMOUNT-1; i >= (int)(CORE_AMOUNT-number_of_cores); i-- )
	{
        p_msg = p_queue_msg[i];
        p_msg->core_id = get_core_id(i);
        p_msg->info.processing_type = pt_cacheinval;

    	p_msg->info.Tvec = TBuf;
    	p_msg->info.Tsize = TSize;
	}

	send_to_cores(pt_cacheinval, number_of_cores, NULL, NULL, NULL);
#endif
}

/**
 * Prepare one shrink image IPC message.
 */
void register_shrink_on_core(const uint8_T *Img, const uint32_T SubArea[4], const uint32_t yStart, const uint32_t yEnd,
		const uint32_t yEvenOdd, const uint32_t yHeight, const uint32_t xWidth, const uint32_t xWidthSmall, uint8_T *ImgSmall, const uint32_T CoreNo)
{
#ifdef _NO_IPC_TEST_
	shrinkImage_on_core(Img, SubArea, yStart, yEnd, yEvenOdd, yHeight, xWidth, xWidthSmall, ImgSmall);
#else

	//To see which entries have to be placed to which parameter look at the union declarations in struct processing_info.
	//Very ugly, I know. But this software is only a scientific proof of concept and it's near its end, hence this kind of code can be written now during the last days ...

	process_message_t * p_msg = 0;

    p_msg = p_queue_msg[CoreNo];
    p_msg->core_id = get_core_id(CoreNo);
    p_msg->info.processing_type = pt_shrink;
    memcpy(p_msg->info.Shr_SubArea, SubArea, sizeof(uint32_T) * 4);
    p_msg->info.Shr_Img=Img;
    p_msg->info.Shr_ImgSmall=ImgSmall;
    p_msg->info.Shr_yStart=yStart;
    p_msg->info.Shr_yEnd=yEnd;
    p_msg->info.Shr_yEvenOdd=yEvenOdd;
    p_msg->info.Shr_yHeight=yHeight;
    p_msg->info.Shr_xWidth=xWidth;
    p_msg->info.Shr_xWidthSmall=xWidthSmall;

#endif
}

/**
 * Send all prepared IPC messages to all cores when shrinking an image.
 */
void start_shrink_on_all_cores(const uint32_T number_of_cores)
{
#ifndef _NO_IPC_TEST_
	send_to_cores(pt_shrink, number_of_cores, NULL, NULL, NULL);
#endif
}

/**
 * Common method for prparing ssd() and/or jacobian() messages for all DSP cores.
 * Divides the row space equally over all cores.
 */
void prepare_ipc_messages_for_all_cores(const processing_type_e ProcessingType, const real32_T w[3], const uint32_T BoundBox[4], const uint32_T MarginAddon[3], const real32_T
             DSPRange[4], const emxArray_uint8_T *Tvec, const uint32_T TOffset, const emxArray_uint8_T *Rvec, const uint32_T ROffset,
             const uint32_T d, const uint32_T number_of_cores)
{
   	const uint32_T i_range = (int32_T)(DSPRange[3]- DSPRange[2] + 1.0f);
    uint32_T i_range_step = i_range/number_of_cores;
    int32_t i;

	#ifdef _TRACE_MC_
	logout("[MAIN  ] i Range=%u, RangeStep=%u\n", i_range, i_range_step);		//trace
	#endif

    int32_T i_from = 0;
    for (i = CORE_AMOUNT-1; i >= (int)(CORE_AMOUNT-number_of_cores); i-- )
	{
		int32_T i_to=i_from+i_range_step;
		#ifdef _TRACE_MC_
			logout("[MAIN  ] i_from=%u, i_to=%u\n", i_from, i_to);		//trace
		#endif
		prepare_ipc_message(ProcessingType, w, BoundBox, MarginAddon, DSPRange, Tvec, TOffset, Rvec, ROffset, d, i, i_from, i_to);
        i_from = i_to;
	}
}

/**
 * Prepare one ssd() and/or jacobian() IPC message.
 */
void prepare_ipc_message(const processing_type_e ProcessingType, const real32_T w[3], const uint32_T BoundBox[4], const uint32_T MarginAddon[3], const real32_T
             DSPRange[4], const emxArray_uint8_T *Tvec, const uint32_T TOffset, const emxArray_uint8_T *Rvec, const uint32_T ROffset,
             const uint32_T d, const uint32_T CoreNo, const uint32_T i_from, const uint32_T i_to)
{
	process_message_t * p_msg = 0;

	p_msg = p_queue_msg[CoreNo];
	p_msg->core_id = get_core_id(CoreNo);
	p_msg->info.processing_type = ProcessingType;
	memcpy(p_msg->info.w, w, sizeof(real32_T) * 3);
	memcpy(p_msg->info.BoundBox, BoundBox, sizeof(uint32_T) * 4);
	memcpy(p_msg->info.MarginAddon, MarginAddon, sizeof(uint32_T) * 3);
	memcpy(p_msg->info.DSPRange, DSPRange, sizeof(real32_T) * 4);
	p_msg->info.Tvec = &Tvec->data[0];
	p_msg->info.Tsize = Tvec->allocatedSize;
	p_msg->info.TOffset = TOffset;
	p_msg->info.Rvec = &Rvec->data[0];
	p_msg->info.Rsize = Rvec->allocatedSize;
	p_msg->info.ROffset = ROffset;
	p_msg->info.d = d;
	p_msg->info.i_from = i_from;
	p_msg->info.i_to = i_to;
	p_msg->info.NewImageDataArrived = g_NewImageDataArrived;
}

MessageQ_Handle h_receive_queue = 0;
int init_message_q()
{
    /* Create the local message queue */
    h_receive_queue = MessageQ_create(MASTER_QUEUE_NAME, NULL);
    if (h_receive_queue == NULL) {
        logout("MessageQ_create failed\n" );
		return -1;
    }
    return 0;
}

MessageQ_QueueId queue_id[MAX_SLICES];
void openSlaveQueues(const uint32_T number_of_cores)
{
    int32_t j;
    int32_t i;

    for (j = 0; j < number_of_cores; j++) {
        do {
            i = MessageQ_open(slave_queue_name[j], &queue_id[j]);
        } while (i < 0);
    }
}

void shutdown_message_q()
{
    if(0 != h_receive_queue)
    {
    	MessageQ_delete(&h_receive_queue);
        h_receive_queue=0;
    }
}

/**
 * Send all prepared IPC messages to all cores and return the calculation result (ssd/jac/hess)
 */
void send_to_cores(const processing_type_e ProcessingType, const uint32_T number_of_cores, real32_T *SSD, real32_T JD[3], real32_T JD2[9])
{
	process_message_t * p_msg = 0;
    uint16_t msgId = 0;
    int32_T ret_val=0;
	#ifdef _TRACE_MC_
    Types_FreqHz freq;
    float processing_time=0;
    Int32 ts1, ts2;
	#endif

    int32_t j;
    int32_t i;

	#ifdef _TRACE_MC_
    logout("[MAIN  ] Execute Process (ProcessingType=%u)\n", ProcessingType);		//trace
    Timestamp_getFreq(&freq);
	#endif

	#ifdef _DO_ERROR_CHECKS_
	if(NULL == h_receive_queue)
	{
		logout("No master msg receive queue available.\n", max_core);
	}

    if ((number_of_cores <= 0) || (number_of_cores > max_core)) {
		logout("Invalid number_of_cores: It should be between 1 to %u\n", max_core);
		ret_val = -1;
		goto mcip_process_error;
    }
	#endif

    //CACHING NOTE:
    //The picture data was cache write backed after images have been received. More
    //data is not to be cache write backed as we pass all other data (also arrays
    //element by element) to the cores using the message queue. Results are passed
    //back also using the message interface as we don't receive bulk data results.

   #ifdef _TRACE_MC_
   ts1 = (Int32) Timestamp_get32();
   #endif

   /* Send messages to processing cores, start at the highest core */
	for (i = CORE_AMOUNT-1; i >= (int)(CORE_AMOUNT-number_of_cores); i-- ) {
	   p_msg = p_queue_msg[i];
       MessageQ_setMsgId(p_msg, ++msgId);
       MessageQ_setReplyQueue(h_receive_queue, (MessageQ_Msg)p_msg);

		#ifdef _TRACE_MC_
		logout("[MAIN  ] Start process on core %u (ProcessingType=%u)\n", p_msg->core_id, ProcessingType, p_msg->info.NewImageDataArrived);		//trace
		#endif

       /* send the message to the remote processor */
       if (MessageQ_put(queue_id[p_msg->core_id], (MessageQ_Msg)p_msg) < 0) {
           logout("MessageQ_put had a failure error\n");
   		ret_val = -1;
   		goto mcip_process_error;
       }
	}

	//All cores have invalidated their cache to read new image data. Next time cache invalidation is no more necessary (until new image data arrives).
	g_NewImageDataArrived = 0;
	#ifdef _TRACE_MC_
		logout("[MAIN  ] Reset g_NetImageDataArrived signal to %d.\n", g_NewImageDataArrived);
	#endif

    //Clear result buffers (will be summed up, have to start at 0)
	if(pt_ssd == ProcessingType || pt_ssdJacHess == ProcessingType)
	{
		(*SSD)=0;
		if(pt_ssdJacHess == ProcessingType)
		{
			memset(JD, 0, sizeof(real32_T) * 3);
			memset(JD2, 0, sizeof(real32_T) * 9);
		}
	}

	//ToDo: Once it looked like all other cores finished calculating before core 0 started. Why ?
	//One could think of having no mcip_core_task at the main core and call the calculation directly instead ... Use _TRACE_MC_ (only) to see this
	//ToDo: When adding a big sleep command to the processing functions one should see if there's something wrong

   /* Receive the result */
   for (i = (CORE_AMOUNT-number_of_cores); i < CORE_AMOUNT; i++) {

       if (MessageQ_get(h_receive_queue, (MessageQ_Msg *)&p_msg, MessageQ_FOREVER) < 0) {
           logout("This should not happen since timeout is forever\n");
   		ret_val = -1;
       }/* else if (p_msg->info.flag != 0) {
           logout("Process image error received from core %d\n", p_msg->core_id);
   		ret_val = -1;
       }*/
		#ifdef _TRACE_MC_
        if(pt_ssd == ProcessingType || pt_ssdJacHess == ProcessingType)
        {
        	logout("[MAIN  ] process answer received from core %u (SSD=%f, ProcessingType=%u)\n", p_msg->core_id, (double)p_msg->info.out_SSD, ProcessingType);		//trace
	    	if(pt_ssdJacHess == ProcessingType)
	    	{
	    		logout("[MAIN  ] JD = [%f %f %f], JD2 = [%f ... %f ... %f]\n", (double)p_msg->info.out_JD[0], (double)p_msg->info.out_JD[1], (double)p_msg->info.out_JD[2],
	    				(double)p_msg->info.out_JD2[0], (double)p_msg->info.out_JD2[4], (double)p_msg->info.out_JD2[8]);
	    	}
   	    }
   	    else
		{
   	    	logout("[MAIN  ] process answer received from core %u (ProcessingType=%u)\n", p_msg->core_id, ProcessingType);		//trace
   	    }
		#endif

		//Sum up the results
	    if(pt_ssd == ProcessingType || pt_ssdJacHess == ProcessingType)
	    {
			(*SSD) += p_msg->info.out_SSD;
			if(pt_ssdJacHess == ProcessingType)
			{
				for(j=0; j<3; j++)
				{
					JD[j] += p_msg->info.out_JD[j];
				}
				for(j=0; j<9; j++)
				{
					JD2[j] += p_msg->info.out_JD2[j];
				}
			}
	    }
   }

   if (ret_val == -1) {
   		goto mcip_process_error;
   }

  #ifdef _TRACE_MC_
   ts2 = (Int32) Timestamp_get32();
   ts2 = ts2 - ts1;
   processing_time = ((float)ts2 / (float)freq.lo);
   if(pt_ssd == ProcessingType || pt_ssdJacHess == ProcessingType)
   {
	   logout("[MAIN  ] SSD calculated in: %f s. Result = %f\n", processing_time, (double)(*SSD));		//trace
	   if(pt_ssdJacHess == ProcessingType)
	   {
		logout("[MAIN  ] JD = [%f %f %f], JD2 = [%f ... %f ... %f]\n", (double)JD[0], (double)JD[1], (double)JD[2], (double)JD2[0], (double)JD2[4], (double)JD2[8]);
	   }
   }
   else
   {
	   logout("[MAIN  ] Image shrinked in: %f s.\n", processing_time);		//trace
   }
   #endif

   return;

   mcip_process_error:
   logout("mcip_process_error !!! \n");
   shutdown_message_q();
}

/**
 * Get the core id for one core (abstraction to enable processing everything on the main core)
 */
uint32_T get_core_id(const uint32_T CoreNo)
{
	#ifdef _ALL_ON_THE_SAME_CORE_TEST_
	return 0;
	#else
	return CoreNo;
	#endif
}

