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

#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <stdint.h>
#include <xdc/std.h>

#include <ti/ipc/Ipc.h>
#include <ti/ipc/MessageQ.h>

#include "rtwtypes.h"
#include "dspreg_types.h"

#define IMAGE_PROCESSING_HEAP_NAME   "image_processing_heap"
#define IMAGE_PROCESSING_HEAPID      0

/*
#define NUMBER_OF_SCRATCH_BUF 3
*/

#define GET_SLAVE_QUEUE_NAME(str, core_id) sprintf(str,"core%d_queue", core_id)

/*
 * Bitmap RGB colormap entry structure.
 */
/*typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t reserved;
} color_table_t;
*/

typedef enum processing_type {
    pt_ssd,
    pt_ssdJacHess,
    pt_shrink,
    pt_cacheinval
} processing_type_e;

/**
 * Take care to cache control every pointer-passed memory buffer ...
 *
 * This struct contains entries for all three IPC methods we use. (Unfortunetrly the TI compiler doesn't support anonymous unions, but space isn't
 * so important here and named unions would clutter the code too much - a refactoring as stated below is better anyway.)
 *
 * TODO: Feel free to refactor this to only send a pointer to a class instance ('command' design pattern), caching must be regarded then.
 * (Meanwhile one could do at least a quick cleanup: Move processing_type to the parent struct and below use two structs instead of one combined (instead
 * of one struct in the member info of the parent struct) when unions won't be used.)
 */
typedef struct processing_info {
    processing_type_e processing_type;

    //SSD/JAC/HESS
    real32_T w[3];
  	uint32_T BoundBox[4];
    uint32_T MarginAddon[3];
    real32_T DSPRange[4];
	const uint8_T* Tvec;
	uint32_T Tsize;
	uint32_T TOffset;
	const uint8_T* Rvec;
	uint32_T Rsize;
	uint32_T ROffset;
	uint32_T d;
	uint32_T i_from;
	uint32_T i_to;
	uint32_t NewImageDataArrived;
    real32_T out_SSD;
    real32_T out_JD[3];
    real32_T out_JD2[9];

    //ShrimkImage
	const uint8_T *Shr_Img;
	uint8_T *Shr_ImgSmall;
	uint32_t Shr_xWidth;
	uint32_T Shr_yHeight;
	uint32_T Shr_yEvenOdd;
	uint32_T Shr_yStart;
	uint32_T Shr_yEnd;
	uint32_T Shr_xWidthSmall;
	uint32_T Shr_SubArea[4];
} processing_info_t;

/* This structure holds the IPC message. The pointers should point to global/shared
 * memory space (like SL2RAM or DDR2).
 * The core_id will always slave core id.
 * In the response message the info.flag will will indicate if there is any failure.
 */
typedef struct process_message {
    MessageQ_MsgHeader  header;
    int                 core_id;
    processing_info_t   info;
} process_message_t;

void process_rgb (processing_info_t * p_info);

#endif /* IMAGE_PROCESSING_H */
