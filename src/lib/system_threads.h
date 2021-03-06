/*
=INSERT_TEMPLATE_HERE=

$Id: system_threads.h,v 1.7 2009/10/26 10:57:07 couannette Exp $

FreeWRL support library.
Internal header: threading library, and processor control (sched).

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#ifndef __LIBFREEWRL_SYSTEM_THREADS_H__
#define __LIBFREEWRL_SYSTEM_THREADS_H__


#if HAVE_PTHREAD
# include <pthread.h>
#endif

#if HAVE_SCHED_H
#include <sched.h>
#endif

/**
 * Threads
 */
#if !defined(WIN32)

#define DEF_THREAD(_t) pthread_t _t = (pthread_t)0
#define TEST_NULL_THREAD(_t) (_t == (pthread_t)0)
#define ID_THREAD(_t) ((unsigned int) _t)
#define ZERO_THREAD(_t) (_t = (pthread_t)0)

#else /* !defined(WIN32) */

#define DEF_THREAD(_t) pthread_t _t = { NULL, 0 }
#define TEST_NULL_THREAD(_t) (_t.p == NULL)
#define ID_THREAD(_t) ((unsigned int) _t.p)
#define ZERO_THREAD(_t) { _t.p = NULL; }

#endif


#endif /* __LIBFREEWRL_SYSTEM_THREADS_H__ */
