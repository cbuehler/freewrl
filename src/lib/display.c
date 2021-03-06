/*
  $Id: display.c,v 1.102 2012/08/09 17:14:29 crc_canada Exp $

  FreeWRL support library.
  Display (X11/Motif or OSX/Aqua) initialization.

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
#include <system.h>
#include <system_threads.h>
#include <internal.h>
#include <display.h>
#include <threads.h>
#include <libFreeWRL.h>

#include "vrml_parser/Structs.h"
#include "opengl/Textures.h"
#include "opengl/RasterFont.h"
#include "opengl/OpenGL_Utils.h"

#include "ui/common.h"

#if defined(FREEWRL_PLUGIN) && (defined(TARGET_X11) || defined(TARGET_MOTIF))
#include "plugin/pluginUtils.h"
#endif


#if defined (TARGET_AQUA)
/* display part specific to Mac */

#ifndef IPHONE

/* for handling Safari window changes at the top of the display event loop */
int PaneClipnpx;
int PaneClipnpy;

int PaneClipct;
int PaneClipcb;
int PaneClipcr;
int PaneClipcl;
int PaneClipwidth;
int PaneClipheight;
int PaneClipChanged = FALSE;
#endif
#endif

void display_init(struct tdisplay* d) 
{
	//public
d->display_initialized = FALSE;
d->win_height = 0; /* window */
d->win_width = 0;
d->winToEmbedInto = -1;
d->fullscreen = FALSE;
d->view_height = 0; /* viewport */
d->view_width = 0;
d->screenWidth = 0; /* screen */
d->screenHeight = 0;
d->screenRatio = 1.5;
d->window_title = NULL;
d->mouse_x = 0;
d->mouse_y = 0;
d->show_mouse = 0;
d->xPos = 0;
d->yPos = 0;
d->shutterGlasses = 0; /* stereo shutter glasses */
d->quadbuff_stereo_mode = 0;
memset(&d->rdr_caps,0,sizeof(d->rdr_caps));
d->myFps = (float) 0.0;

//private
} 



#if KEEP_FV_INLIB

#if defined (_ANDROID)

/* simple display initialize for Android (and, probably, iPhones, too) */

int fv_display_initialize()
{
    struct tdisplay* d = &gglobal()->display;
    
    //printf ("fv_display_initialize called\n");
    if (d->display_initialized) {
        //ConsoleMessage ("fv_display_initialized re-called for a second time");
        return TRUE;
    }
    
    if (!fwl_initialize_GL()) {
        return FALSE;
    }
    
    /* Display full initialized :P cool ! */
    d->display_initialized = TRUE;
   
    PRINT_GL_ERROR_IF_ANY ("end of fv_display_initialize");
    
#ifdef DO_COLLISION_GPU
    collision_initGPUCollide();
#endif
    
    return TRUE;
}
#else
/**
 *  fv_display_initialize: takes care of all the initialization process, 
 *                      creates the display thread and wait for it to complete
 *                      the OpenGL initialization and the Window creation.
 */
int fv_display_initialize()
{
	struct tdisplay* d = &gglobal()->display;
	if (d->display_initialized) return TRUE;

	//memset(&d->rdr_caps, 0, sizeof(d->rdr_caps));

	/* FreeWRL parameters */
	d->fullscreen = fwl_getp_fullscreen();
	d->win_width = fwl_getp_width();
	d->win_height = fwl_getp_height();
	d->winToEmbedInto = fwl_getp_winToEmbedInto();

	/* make the window, get the OpenGL context */
#if !defined(_MSC_VER) && !defined(GLES2)
	if (!fv_open_display()) {
		return FALSE;
	}

	if (!fv_create_GLcontext()) {
		return FALSE;
	}

#endif

	if (0 != d->screenWidth)  d->win_width  = d->screenWidth;
	if (0 != d->screenHeight) d->win_height = d->screenHeight;
	fv_setScreenDim(d->win_width,d->win_height); /* recompute screenRatio */

	//snprintf(window_title, sizeof(window_title), "FreeWRL");

	if (!fv_create_main_window(0 /*argc*/, NULL /*argv*/)) {
		return FALSE;
	}

	setWindowTitle0();

#if ! ( defined(_MSC_VER) || defined(FRONTEND_HANDLES_DISPLAY_THREAD) )
	fv_bind_GLcontext();
#endif

	if (!fwl_initialize_GL()) {
		return FALSE;
	}
        /* lets make sure everything is sync'd up */
#if defined(TARGET_X11) || defined(TARGET_MOTIF)
        XFlush(Xdpy);
#endif

        /* initialize default font 
           
           TODO: this may be a configuration option (config file or command line)
         */
        rf_xfont_init("fixed");

	/* Display full initialized :P cool ! */
	d->display_initialized = TRUE;

	DEBUG_MSG("FreeWRL: running as a plugin: %s\n", BOOL_STR(isBrowserPlugin));

    PRINT_GL_ERROR_IF_ANY ("end of fv_display_initialize");
    
#if !(defined(TARGET_AQUA) || defined(_MSC_VER) || defined(_ANDROID) || defined(GLES2))
        
	if (RUNNINGASPLUGIN) {
#if defined(FREEWRL_PLUGIN) && (defined(TARGET_X11) || defined(TARGET_MOTIF))
		sendXwinToPlugin();
#endif
	} else {
		XMapWindow(Xdpy, Xwin);
	}
#endif /* IPHONE */
#ifdef DO_COLLISION_GPU
		collision_initGPUCollide();
#endif

	return TRUE;
}
#endif //ANDROID


/**
 *   fv_setGeometry_from_cmdline: scan command line arguments (X11 convention), to
 *                             set up the window dimensions.
 */
int fwl_parse_geometry_string(const char *geometry, int *out_width, int *out_height, 
			      int *out_xpos, int *out_ypos)
{
	int width, height, xpos, ypos;
	int c;

	width = height = xpos = ypos = 0;

	c = sscanf(geometry, "%dx%d+%d+%d", 
		   &width, &height, &xpos, &ypos);

	if (out_width) *out_width = width;
	if (out_height) *out_height = height;
	if (out_xpos) *out_xpos = xpos;
	if (out_ypos) *out_ypos = ypos;

	if (c > 0)
		return TRUE;
	return FALSE;
}

void fv_setScreenDim(int wi, int he) { fwl_setScreenDim(wi,he); }
#endif /* KEEP_FV_INLIB */

/**
 *   fwl_setScreenDim: set internal variables for screen sizes, and calculate frustum
 */
void fwl_setScreenDim(int wi, int he)
{
    gglobal()->display.screenWidth = wi;
    gglobal()->display.screenHeight = he;
    /* printf("%s,%d fwl_setScreenDim(int %d, int %d)\n",__FILE__,__LINE__,wi,he); */

    if (gglobal()->display.screenHeight != 0) gglobal()->display.screenRatio = (double) gglobal()->display.screenWidth/(double) gglobal()->display.screenHeight;
    else gglobal()->display.screenRatio =  gglobal()->display.screenWidth;
}

/**
 *   resize_GL: when the window is resized we have to update the GL viewport.
 */
GLvoid resize_GL(GLsizei width, GLsizei height)
{ 
    FW_GL_VIEWPORT( 0, 0, width, height ); 
	printf("resize_GL\n");
}

/**
 * On all platforms, when we don't have GLEW, we simulate it.
 * In any case we setup the rdr_capabilities struct.
 */
bool initialize_rdr_caps()
{
	s_renderer_capabilities_t rdr_caps;

#ifdef HAVE_LIBGLEW

	/* Initialize GLEW */
	{
	GLenum err;
	err = glewInit();
	if (GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		ERROR_MSG("GLEW initialization error: %s\n", glewGetErrorString(err));
		return FALSE;
	}
	TRACE_MSG("GLEW initialization: version %s\n", glewGetString(GLEW_VERSION));
	}
#endif
	/* OpenGL is initialized, context is created,
	   get some info, for later use ...*/
        rdr_caps.renderer   = (char *) FW_GL_GETSTRING(GL_RENDERER);
        rdr_caps.version    = (char *) FW_GL_GETSTRING(GL_VERSION);
        rdr_caps.vendor     = (char *) FW_GL_GETSTRING(GL_VENDOR);
	rdr_caps.extensions = (char *) FW_GL_GETSTRING(GL_EXTENSIONS);
    FW_GL_GETBOOLEANV(GL_STEREO,&(rdr_caps.quadBuffer));
    //if (rdr_caps.quadBuffer) ConsoleMessage("INIT HAVE QUADBUFFER"); else ConsoleMessage("INIT_ NO QUADBUFFER");
    

                      

	/* rdr_caps.version = "1.5.7"; //"1.4.1"; //for testing */
    rdr_caps.versionf = (float) atof(rdr_caps.version); 
	/* atof technique: http://www.opengl.org/resources/faq/technical/extensions.htm */
    rdr_caps.have_GL_VERSION_1_1 = rdr_caps.versionf >= 1.1f;
    rdr_caps.have_GL_VERSION_1_2 = rdr_caps.versionf >= 1.2f;
    rdr_caps.have_GL_VERSION_1_3 = rdr_caps.versionf >= 1.3f;
    rdr_caps.have_GL_VERSION_1_4 = rdr_caps.versionf >= 1.4f;
    rdr_caps.have_GL_VERSION_1_5 = rdr_caps.versionf >= 1.5f;
    rdr_caps.have_GL_VERSION_2_0 = rdr_caps.versionf >= 2.0f;
    rdr_caps.have_GL_VERSION_2_1 = rdr_caps.versionf >= 2.1f;
    rdr_caps.have_GL_VERSION_3_0 = rdr_caps.versionf >= 3.0f;

#ifdef HAVE_LIBGLEW

	{
    printf("opengl version=%s\n",rdr_caps.version);
	if(!rdr_caps.have_GL_VERSION_2_0)
	{
		printf("please upgrade to opengl version 2+\n see http://www.opengl.org/wiki/Getting_Started  for details\n");
		printf("press enter to exit\n");
		getchar();
		exit(-1);
	}


	rdr_caps.av_glsl_shaders = GLEW_ARB_fragment_shader;
	rdr_caps.av_multitexture = GLEW_ARB_multitexture;
	rdr_caps.av_occlusion_q = GLEW_ARB_occlusion_query;
	rdr_caps.av_npot_texture = GLEW_ARB_texture_non_power_of_two;
	rdr_caps.av_texture_rect = GLEW_ARB_texture_rectangle;
	}

#else
	/* Initialize renderer capabilities without GLEW */

	/* Shaders */
        rdr_caps.av_glsl_shaders = (strstr (rdr_caps.extensions, "GL_ARB_fragment_shader")!=0);
	
	/* Multitexturing */
	rdr_caps.av_multitexture = (strstr (rdr_caps.extensions, "GL_ARB_multitexture")!=0);

	/* Occlusion Queries */
	rdr_caps.av_occlusion_q = (strstr (rdr_caps.extensions, "GL_ARB_occlusion_query") !=0);

	/* Non-power-of-two textures */
	rdr_caps.av_npot_texture = (strstr (rdr_caps.extensions, "GL_ARB_texture_non_power_of_two") !=0);

	/* Texture rectangle (x != y) */
	rdr_caps.av_texture_rect = (strstr (rdr_caps.extensions, "GL_ARB_texture_rectangle") !=0);
#endif

	/* if we are doing our own shading, force the powers of 2, because otherwise mipmaps are not possible. */
	rdr_caps.av_npot_texture=FALSE;

	/* attempting multi-texture */
	rdr_caps.av_multitexture = 1;

	/* Max texture size */
	{
		GLint tmp;  /* ensures that we pass pointers of same size across all platforms */
		
		FW_GL_GETINTEGERV(GL_MAX_TEXTURE_SIZE, &tmp);
		rdr_caps.max_texture_size = (int) tmp;

		#ifdef GL_ES_VERSION_2_0
		FW_GL_GETINTEGERV(GL_MAX_TEXTURE_IMAGE_UNITS, &tmp);
		#else
		FW_GL_GETINTEGERV(GL_MAX_TEXTURE_UNITS, &tmp);
		#endif
		rdr_caps.texture_units = (int) tmp;
	}

	/* max supported texturing anisotropicDegree- can be changed in TextureProperties */
#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
	FW_GL_GETFLOATV (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &rdr_caps.anisotropicDegree);
#endif
	/* User settings in environment */

	if (gglobal()->internalc.global_texture_size > 0) {
		DEBUG_MSG("Environment set texture size: %d", gglobal()->internalc.global_texture_size);
		rdr_caps.max_texture_size = gglobal()->internalc.global_texture_size;
	}

	/* Special drivers settings */
	if (
	strstr(rdr_caps.renderer, "Intel GMA 9") != NULL ||
	strstr(rdr_caps.renderer, "Intel(R) 9") != NULL ||
	strstr(rdr_caps.renderer, "i915") != NULL ||
	strstr(rdr_caps.renderer, "NVIDIA GeForce2") != NULL
	) {
		if (rdr_caps.max_texture_size > 1024) rdr_caps.max_texture_size = 1024;
	}

	/* print some debug infos */
	rdr_caps_dump(&rdr_caps);

	//make this the renderer caps for this thread.
	memcpy(&gglobal()->display.rdr_caps,&rdr_caps,sizeof(rdr_caps));
	return TRUE;
}

void initialize_rdr_functions()
{
	/**
	 * WARNING:
	 *
	 * Linux OpenGL driver (Mesa or ATI or NVIDIA) and Windows driver
	 * will not initialize function pointers. So we use GLEW or we
	 * initialize them ourself.
	 *
	 * OSX 10.4 : same as above.
	 * OSX 10.5 and OSX 10.6 : Apple driver will initialize functions.
	 */

	
}

void rdr_caps_dump(s_renderer_capabilities_t *rdr_caps)
{
#ifdef VERBOSE
	{
		char *p, *pp;
		p = pp = STRDUP(rdr_caps->extensions);
		while (*pp != '\0') {
			if (*pp == ' ') *pp = '\n';
			pp++;
		}
		DEBUG_MSG ("OpenGL extensions : %s\n", p);
		FREE(p);
	}
#endif //VERBOSE

	DEBUG_MSG ("Shader support:       %s\n", BOOL_STR(rdr_caps->av_glsl_shaders));
	DEBUG_MSG ("Multitexture support: %s\n", BOOL_STR(rdr_caps->av_multitexture));
	DEBUG_MSG ("Occlusion support:    %s\n", BOOL_STR(rdr_caps->av_occlusion_q));
	DEBUG_MSG ("Max texture size      %d\n", rdr_caps->max_texture_size);
	DEBUG_MSG ("Texture units         %d\n", rdr_caps->texture_units);
}

/* Local Variables: */
/* c-basic-offset: 8 */
/* End: */
