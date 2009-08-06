/*
=INSERT_TEMPLATE_HERE=

$Id: OpenGL_Utils.h,v 1.8 2009/08/06 20:10:11 crc_canada Exp $

Screen snapshot.

*/

#ifndef __FREEWRL_OPENGL_UTILS_H__
#define __FREEWRL_OPENGL_UTILS_H__


void start_textureTransform (struct X3D_Node *textureNode, int ttnum);
void end_textureTransform (void);

void
glpOpenGLInitialize(void);


void
BackEndClearBuffer(void);

void
BackEndLightsOff(void);

void lightState (GLint light, int state);


extern void glpOpenGLInitialize(void);

extern void glPrintError(char *);
void drawBBOX(struct X3D_Node *node);

extern int opengl_has_shaders;
extern int opengl_has_multitexture;
extern int opengl_has_occlusionQuery;
extern int opengl_has_numTextureUnits;
extern GLint opengl_has_textureSize;

#endif /* __FREEWRL_OPENGL_UTILS_H__ */
