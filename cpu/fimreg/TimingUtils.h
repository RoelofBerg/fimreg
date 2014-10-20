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

#ifndef TimingUtils_h
#define TimingUtils_h 

//  Windows
#ifdef _WIN32
#include <Windows.h>

inline double get_wall_time()
{
    LARGE_INTEGER time,freq;
    if (!QueryPerformanceFrequency(&freq))
	{
        //  Handle error
        return 0;
    }
    if (!QueryPerformanceCounter(&time))
	{
        //  Handle error
        return 0;
    }
    return (double)time.QuadPart / freq.QuadPart;
}

inline double get_cpu_time()
{
    FILETIME CreationTime, ExitTime, KernelTime, UserTime;

    if (GetProcessTimes(GetCurrentProcess(), &CreationTime, &ExitTime, &KernelTime, &UserTime) != 0)
	{
        // Returns the sum of user and kernel time.
		double UserSeconds = (double)(UserTime.dwLowDateTime | ((unsigned long long)UserTime.dwHighDateTime << 32)) * 0.0000001;
		double KernelSeconds = (double)(KernelTime.dwLowDateTime | ((unsigned long long)KernelTime.dwHighDateTime << 32)) * 0.0000001;
        return UserSeconds + KernelSeconds;
    }
	else
	{
        //  Handle error
        return 0;
    }
}

//  Posix/Linux
#else

#include <sys/time.h>
inline double get_wall_time()
{
    struct timeval time;
    if (gettimeofday(&time,NULL))
	{
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

inline double get_cpu_time()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

#endif

#endif //TimingUtils_h
