/*
 * FreeX3D plugin
 *
 * To communicate with FreeX3D, we define structures and protocol.
 *
 */

#ifndef __FREEX3D_PLUGIN_UTILS_H__
#define __FREEX3D_PLUGIN_UTILS_H__


#ifndef _DEBUG
#define _DEBUG 0
#endif

#define SMALLSTRINGSIZE 64
#define STRINGSIZE 128
#define LARGESTRINGSIZE 256

#define PLUGIN_PORT 2009
#define PLUGIN_TIMEOUT_SEC 10
#define PLUGIN_TIMEOUT_NSEC 0

#define PLUGIN_RETRY 2
#define SLEEP_TIME 5

#define NO_ERROR 0
#define SOCKET_ERROR -1000
#define SIGNAL_ERROR -1001

#define UNUSED(v) ((void) v)

typedef struct _urlRequest {
    char url[FILENAME_MAX]; /* limit url length (defined in stdio.h) */
    void *instance;   /* NPP instance for plugin */
    unsigned int notifyCode; /* NPN_GetURLNotify, NPP_URLNotify */
} urlRequest;

/* const char* XEventToString(int type); */
/* const char* XErrorToString(int error); */

void URLencod (char *dest, const char *src, int maxlen);


#endif /* __FREEX3D_PLUGIN_UTILS_H__ */
