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

/**
*  @file sensorclient/main.cpp
*  \brief Implementation of the applications main entry point for 'fimreg'.
*
*  \author Roelof Berg
*/

#include "stdafx.h"	//Precompiled headerfile, MUST be first entry in .cpp file
#include "CRegistrationController.h"
#include "CRegistrator.h"
#include "CHPRPCConnection.h"

/**
 * Entry point for the application.
 * (Also place for the startup message and main exception handler).
 */
int main(int argc, char ** argv)
{
	#if defined(WIN32) && !defined(_DEBUG)
	//Set process priority to max.
	if(!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
	{
		printf("Cannot set process prioroty to REALTIME.\n");
	}
	printf("Operating on REALTIME priority.\n");
	#endif

	//Run application
	int retVal = 0;
	try
	{
		CLogger::PrintStartupMessage(argv[0]);

		//Application wiring and application start
		//(Application wiring means connecting the object instances. Instances will be created here or in a unit
		//test. The objects tend not to create instances. This enhances unit testability. If an object has to
		//create an instance it should use a factory class. If the unit test framework does not support object
		//replacement one must use interfaces (abstract base classes).)

		//We use references here instead of pointer logic to let the compiler check for valid reference instances
		//Note: Compiler will automatically place large objects on the heap.
		CRegistrator oRegistrator;
		CRegistrationController oApplication(oRegistrator);

		retVal = oApplication.Main(argc, argv);
	}
	catch (exception& e)
	{
		CLogger::PrintError(e.what());
		CLogger::PrintError(APP_ERR_EXCEPTION);
		retVal = APP_RET_ERROR;
	}
	catch (...)
	{
		CLogger::PrintError(APP_ERR_EXCEPTION);
		retVal = APP_RET_ERROR;
	}

	return retVal;
}
