/*******************************************************************
 *
 * FreeWRL main program
 *
 * internal header - system.h
 *
 * Program system dependencies.
 *
 * $Id: system.h,v 1.8 2009/08/19 04:07:34 dug9 Exp $
 *
 *******************************************************************/

#ifndef __FREEWRL_SYSTEM_H__
#define __FREEWRL_SYSTEM_H__


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

#if defined(_MSC_VER)

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

#if HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif

#if HAVE_SYS_MSG_H
# include <sys/msg.h>
#endif

#if !defined(assert)
# include <assert.h>
#endif

#if HAVE_DIRECT_H
#include <direct.h>
#endif

#if HAVE_SIGNAL_H 
#include <signal.h>
    /* install the signal handler for SIGQUIT */
#define SIGQUIT SIGINT
/*#define SIGTERM SIGTERM  *//*not generated under win32 but can raise */
/*#define SIGSEGV SIGSEGV */  /* memory overrun */
#define SIGALRM SIGABRT  /* I don't know so I guessed the lookup */
#define SIGHUP SIGFPE   /* fpe means floating poinot error */
#endif


#else

#if HAVE_SIGNAL_H 
#include <signal.h>
#endif

#endif



#endif /* __FREEWRL_SYSTEM_H__ */
