/*********************************************************************
 *
 * FreeWRL plugin
 *
 *********************************************************************/

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



#ifndef __FREEWRL_PLUGIN_SYSTEM_H__
#define __FREEWRL_PLUGIN_SYSTEM_H__


#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_STDINT_H
# include <stdint.h>
#endif

#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
# if ! HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
typedef unsigned char _Bool;
#  endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif

#define BOOL _Bool
#define TRUE 1
#define FALSE 0

#if HAVE_UNISTD_H
# include <sys/types.h>
# include <unistd.h>
#endif

#if HAVE_ERRNO_H
# include <errno.h>
#endif

#if HAVE_TIME_H
# include <time.h>
#endif

#if HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif

#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifndef HAVE_INT32
# define int32 int32_t
#endif
#ifndef HAVE_UINT32
# define uint32 uint32_t
#endif
#ifndef HAVE_INT16
# define int16 int16_t
#endif
#ifndef HAVE_UINT16
# define uint16 uint16_t
#endif

#endif /* __FREEWRL_PLUGIN_SYSTEM_H__ */
