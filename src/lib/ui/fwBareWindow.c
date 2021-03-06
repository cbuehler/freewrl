/*
  $Id: fwBareWindow.c,v 1.18 2012/05/31 18:44:44 crc_canada Exp $

  Create X11 window. Manage events.

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

#include <config.h>

#if !(defined(IPHONE) || defined(_ANDROID) || defined(AQUA))

#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "fwBareWindow.h"

XTextProperty windowName;
Window Pwin;
static XSetWindowAttributes Swa;

/* we have no buttons; just make these functions do nothing */
void frontendUpdateButtons() {}
/* void setMenuButton_collision (int val) {} */
/* void setMenuButton_headlight (int val) {} */
/* void setMenuButton_navModes (int type) {} */
/* void setMenuButton_texSize (int size) {} */

void getBareWindowedGLwin (Window *win)
{
    GLwin = Xwin;
}

void openBareMainWindow (int argc, char **argv)
{
    /* get a connection */
    Xdpy = XOpenDisplay(0);
    if (!Xdpy) { fprintf(stderr, "No display!\n");exit(-1);}
}

int fv_create_main_window(int argc, char *argv[])
{
    /* Window root_ret; */
    /* Window child_ret; */
    /* int root_x_ret; */
    /* int root_y_ret; */
    /* unsigned int mask_ret; */

    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = colormap;

    attr.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | LeaveWindowMask | MapNotify | ButtonPressMask | ButtonReleaseMask | FocusChangeMask;

    if (fwl_params.fullscreen) {
	mask = CWBackPixel | CWColormap | CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWEventMask;
	attr.override_redirect = true;
	attr.backing_store = NotUseful;
	attr.save_under = false;
    } else {
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
    }
		
    Xwin = XCreateWindow(Xdpy, Xroot_window, 0, 0, 
			 fwl_params.width, fwl_params.height,
			 0, Xvi->depth, InputOutput, Xvi->visual, mask, &attr);

    /* FIXME: Caller / front-end should reparent FreeWRL window itself */
    /* Roberto Gerson */
    /* If -d is setted, so reparent the window */
    if (fwl_params.winToEmbedInto > 0) {
	    DEBUG_MSG("create_main_window: reparent %ld to %ld\n",
		      Xwin,
		      fwl_params.winToEmbedInto);
	    XReparentWindow(Xdpy, Xwin, (Window) fwl_params.winToEmbedInto, 0, 0);
    }


    if (!RUNNINGASPLUGIN) {
	    XMapWindow(Xdpy, Xwin);
	    XFlush(Xdpy);
    }
		
    if (fwl_params.fullscreen) {
	XMoveWindow(Xdpy, Xwin, 0, 0);
	XRaiseWindow(Xdpy, Xwin);
	XFlush(Xdpy);
#ifdef HAVE_XF86_VMODE
	XF86VidModeSetViewPort(Xdpy, Xscreen, 0, 0);
#endif
	XGrabPointer(Xdpy, Xwin, TRUE, 0, GrabModeAsync, GrabModeAsync, Xwin, None, CurrentTime);
	XGrabKeyboard(Xdpy, Xwin, TRUE, GrabModeAsync, GrabModeAsync, CurrentTime);
    } else {
	WM_DELETE_WINDOW = XInternAtom(Xdpy, "WM_DELETE_WINDOW", FALSE);
	XSetWMProtocols(Xdpy, Xwin, &WM_DELETE_WINDOW, 1);
    }
		
    /* XQueryPointer(Xdpy, Xwin, &root_ret, &child_ret, &root_x_ret, &root_y_ret, */
		  /* &mouse_x, &mouse_y, &mask_ret); */

    GLwin = Xwin;
		
    XFlush(Xdpy);

    return TRUE;
}

#endif /* IPHONE */

/* Local Variables: */
/* c-basic-offset: 8 */
/* End: */
