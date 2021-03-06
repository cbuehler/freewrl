/*
=INSERT_TEMPLATE_HERE=

$Id: RenderFuncs.h,v 1.32 2012/07/25 18:45:27 crc_canada Exp $

Proximity sensor macro.

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


#ifndef __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__
#define __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__

//OLDCODE void enableGlobalShader (shader_type_t);

void enableGlobalShader(s_shader_capabilities_t *);

void turnGlobalShaderOff(void);

#ifdef GL_VERSION_2_0
	#define TURN_GLOBAL_SHADER_OFF \
		turnGlobalShaderOff()
	#define TURN_FILLPROPERTIES_SHADER_OFF \
		{if (p->fillpropCurrentShader!=0) { glUseProgram(0);}}
#else
	#ifdef GL_VERSION_1_5
		#define TURN_GLOBAL_SHADER_OFF \
		turnGlobalShaderOff()
		#define TURN_FILLPROPERTIES_SHADER_OFF \
			{if (fillpropCurrentShader!=0) { fillpropCurrentShader = 0; glUseProgramObjectARB(0);}}
	#else
		#if defined(IPHONE) || defined(GLES2)
			#define TURN_GLOBAL_SHADER_OFF \
				turnGlobalShaderOff()
			#define TURN_FILLPROPERTIES_SHADER_OFF \
				{if (fillpropCurrentShader!=0) { glUseProgram(0);}}

		#else
			#define TURN_GLOBAL_SHADER_OFF printf ("can not do TURN_SHADERS_OFF at %s:%d\n",__FILE__,__LINE__);
			#define TURN_FILLPROPERTIES_SHADER_OFF printf ("can not do TURN_SHADERS_OFF at %s:%d\n",__FILE__,__LINE__);
		#endif
	#endif
#endif

/* trat: test if a ratio is reasonable */
#define TRAT(a) ((a) > 0 && ((a) < gglobal()->RenderFuncs.hitPointDist || gglobal()->RenderFuncs.hitPointDist < 0))

/* structure for rayhits */
struct currayhit {
	struct X3D_Node *hitNode; /* What node hit at that distance? */
	GLDOUBLE modelMatrix[16]; /* What the matrices were at that node */
	GLDOUBLE projMatrix[16];
};

extern struct point_XYZ r1, r2;         /* in VRMLC.pm */


/* function protos */
int nextlight(void);
void render_node(struct X3D_Node *node);

struct X3D_Anchor *AnchorsAnchor();
void setAnchorsAnchor(struct X3D_Anchor* anchor);

void lightState(GLint light, int status);
void saveLightState(int *ls);
void restoreLightState(int *ls);
void fwglLightfv (int light, int pname, GLfloat *params);
void fwglLightf (int light, int pname, GLfloat param);
void initializeLightTables(void);
void sendAttribToGPU(int myType, int mySize, int  xtype, int normalized, int stride, float *pointer, char*, int);
void sendClientStateToGPU(int enable, int cap);
void sendArraysToGPU (int mode, int first, int count);
void sendBindBufferToGPU (GLenum target, GLuint buffer,char *, int);
void sendElementsToGPU (int mode, int count, int type, int *indices);
void render_hier(struct X3D_Group *p, int rwhat);
void sendLightInfo (s_shader_capabilities_t *me);

#endif /* __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__ */
