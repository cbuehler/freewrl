/*******************************************************************
 *
 * FreeX3D support library
 *
 * internal header - system.h
 *
 * Library system dependencies.
 *
 * $Id: system.h,v 1.4 2008/11/27 00:27:17 couannette Exp $
 *
 *******************************************************************/

#ifndef __LIBFREEX3D_SYSTEM_H__
#define __LIBFREEX3D_SYSTEM_H__

#if HAVE_STDINT_H
# include <stdint.h>
#endif

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

#if HAVE_ERRNO_H
# include <errno.h>
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

#define TRUE 1
#define FALSE 0

#if HAVE_UNISTD_H
# include <sys/types.h>
# include <unistd.h>
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

#if HAVE_PTHREAD
# include <pthread.h>
#endif

/**
 * TODO:
 * finish the armor job...
 */
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <syslog.h>
#include <stdarg.h>

#if HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <sys/time.h>
#include <time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <syslog.h>

#include <fcntl.h>

#include <netinet/in.h>
#include <sys/socket.h>


#endif /* __LIBFREEX3D_SYSTEM_H__ */
