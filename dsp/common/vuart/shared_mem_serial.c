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

#ifndef _PCIE_VUART_DISABLED_

#define WATCHDOG_RESET()

#include "shared_mem_serial.h"

typedef volatile struct _shm_cbuf_t {
	unsigned	out_start;	/* Output buffer start */
	unsigned	out_end;	/* Output buffer end + 1 */
	unsigned	out_put;	/* Output put offset */
	unsigned	out_get;	/* Output get offset */
	unsigned	in_start;	/* Output buffer start */
	unsigned	in_end;		/* Output buffer end + 1 */
	unsigned	in_put;		/* Output put offset */
	unsigned	in_get;		/* Output get offset */
} shm_cbuf_t;

#include <xdc/cfg/global.h>	//constants from .cfg file like CFG_SHM_CONSOLE_OUTSZ, CFG_SHM_CONSOLE_INSZ
#pragma DATA_SECTION (g_vuart_dma_memory, ".vuartDMA");
unsigned char g_vuart_dma_memory[sizeof(shm_cbuf_t) + CFG_SHM_CONSOLE_OUTSZ + CFG_SHM_CONSOLE_INSZ];

/*------------------------------------------------------------------------
 * SHM CONSOLE -- the shared memory console is useful when debugging and
 * at least offers some mechanism for user I/O.
 *
 * When the error vuart init fails error 5 comes in the client application
 * two causes can happen:
 * a) Client and DSP use different memory regions (see .vuartDMA losding in .cfg)
 * b) Someone uses uart_printf (logout) before the vuart has been initialized (e.g. ctor of global variable)
 *
 *----------------------------------------------------------------------*/

static shm_cbuf_t *cbuf;

void serial_setbrg (void){ return; }

int serial_init (void)
{
	cbuf = (shm_cbuf_t *)(&g_vuart_dma_memory[0]);

	cbuf->out_start = sizeof (shm_cbuf_t);
	cbuf->out_end = cbuf->out_start + CFG_SHM_CONSOLE_OUTSZ;
	cbuf->out_put = cbuf->out_start;
	cbuf->out_get = cbuf->out_start;

	cbuf->in_start = cbuf->out_end;
	cbuf->in_end = cbuf->in_start + CFG_SHM_CONSOLE_INSZ;
	cbuf->in_put = cbuf->in_start;
	cbuf->in_get = cbuf->in_start;

	return (0);
}

void serial_putc (char c)
{
	unsigned next_put;
	char *put = (char *)cbuf + cbuf->out_put;

	/* Determine where the subsequent write (not this one) will
	 * be -- we can never allow put = get when writing.
	 */
	next_put = cbuf->out_put + 1;
	if (next_put >= cbuf->out_end)
		next_put = cbuf->out_start;
	while (next_put == cbuf->out_get)
		WATCHDOG_RESET ();

	/* There is room in the buffer to write the char */
	*put = c;
	cbuf->out_put = next_put;

	return;
}

void serial_puts (const char *s)
{
	while (*s != 0)
		serial_putc (*s++);
}

int serial_tstc (void)
{
	return (cbuf->in_put != cbuf->in_get);
}

int serial_getc (void)
{

	char *get;
	int c;

retry:
	get = (char *)cbuf + cbuf->in_get;
	while (cbuf->in_get == cbuf->in_put)
		WATCHDOG_RESET ();

	c = *get & 0x0ff;
	if (cbuf->in_get + 1 >= cbuf->in_end)
		cbuf->in_get = cbuf->in_start;
	else
		cbuf->in_get += 1;
	
	if(c == 10)
		goto retry;
	return (c);
}

#endif //_PCIE_VUART_DISABLED_
