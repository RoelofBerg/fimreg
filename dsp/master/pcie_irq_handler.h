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

#ifndef _HPRPC_SERVER_H_
#define _HPRPC_SERVER_H_

#include <xdc/std.h>

#define CPINTC_0 0
#define HOST_INT_ID 8
#define INTC_VECTID 4
#define TI667X_IRQ_EOI			0x21800050			/* End of Interrupt Register */
#define TI667X_EP_IRQ_SET		0x21800064			/* Endpoint Interrupt Request Set Register */
#define TI667X_EP_IRQ_CLR		0x21800068			/* Endpoint Interrupt Request Clear Register */
#define TI667X_EP_IRQ_STATUS	0x2180006C			/* Endpoint Interrupt Stauts Register */

#define MSI0_IRQ_STATUS_RAW		0x21800100			/* MSI 0 Raw Interrupt Status Register */
#define MSI0_IRQ_STATUS			0x21800104			/* MSI 0 Interrupt Enabled Status Register */
#define LEGACY_A_IRQ_STATUS_RAW	0x21800180			/* Raw Interrupt Status Register */
#define LEGACY_A_IRQ_STATUS		0x21800184			/* Interrupt Enabled Status Register */
#define LEGACY_B_IRQ_STATUS_RAW	0x21800190			/* Raw Interrupt Status Register */
#define LEGACY_B_IRQ_STATUS		0x21800194			/* Interrupt Enabled Status Register */

extern void hprpc_irq_handler_init ();
extern void hprpc_irq_handler (UArg Params);
void hprpc_serve_irqs();

#endif //_HPRPC_SERVER_H_
