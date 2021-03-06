/*
=INSERT_TEMPLATE_HERE=

$Id: Component_Geometry3D.c,v 1.91 2012/07/18 13:54:46 crc_canada Exp $

X3D Geometry 3D Component

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
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../opengl/Frustum.h"
#include "../opengl/Textures.h"
#include "../opengl/OpenGL_Utils.h"
#include "../scenegraph/Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"


#include "Collision.h"
#include "Polyrep.h"
#include "LinearAlgebra.h"
#include "Component_Geometry3D.h"


/* used for vertices for VBOs. Note the tc coordinate... */
struct MyVertex
 {
   struct SFVec3f vert;        //Vertex
   struct SFVec3f norm;     //Normal
   struct SFVec2f tc;         //Texcoord0
 };

typedef struct pComponent_Geometry3D{
	int junk; //filler, no variables unless/untill vbos done
	//SphereGeomVBO ShpereIndxVBO - are these per-instance or sharable?
}* ppComponent_Geometry3D;
void *Component_Geometry3D_constructor(){
	void *v = malloc(sizeof(struct pComponent_Geometry3D));
	memset(v,0,sizeof(struct pComponent_Geometry3D));
	return v;
}
void Component_Geometry3D_init(struct tComponent_Geometry3D *t){
	//public
	//private
	t->prv = Component_Geometry3D_constructor();
	// JAS {ppComponent_Geometry3D p = (ppComponent_Geometry3D)t->prv;}
}
//ppComponent_Geometry3D p = (ppComponent_Geometry3D)gglobal()->Component_Geometry3D.prv;






static GLfloat VBO_coneSideTexParams[]={
	0.55f, 0.525f, 0.50f,
	0.60f, 0.575f, 0.55f,
	0.65f, 0.625f, 0.60f,
	0.70f, 0.675f, 0.65f,
	0.75f, 0.725f, 0.70f,
	0.80f, 0.775f, 0.75f,
	0.85f, 0.825f, 0.80f,
	0.90f, 0.875f, 0.85f,
	0.95f, 0.925f, 0.90f,
	1.00f, 0.975f, 0.95f,
	0.05f, 0.025f, 0.00f,
	0.10f, 0.075f, 0.05f,
	0.15f, 0.125f, 0.10f,
	0.20f, 0.175f, 0.15f,
	0.25f, 0.225f, 0.20f,
	0.30f, 0.275f, 0.25f,
	0.35f, 0.325f, 0.30f,
	0.40f, 0.375f, 0.35f,
	0.45f, 0.425f, 0.40f,
	0.50f, 0.475f, 0.45f,
	0.55f, 0.525f, 0.50f
};

/*******************************************************************************/

/*  have to regen the shape*/
void compile_Box (struct X3D_Box *node) {
	struct SFVec3f *pt;
	struct SFVec3f *ptr;
	float x = ((node->size).c[0])/2;
	float y = ((node->size).c[1])/2;
	float z = ((node->size).c[2])/2;

	MARK_NODE_COMPILED

	/*  MALLOC memory (if possible)*/
	if (!node->__points.p) ptr = MALLOC (struct SFVec3f *,sizeof(struct SFVec3f)*(36));
	else ptr = node->__points.p;

	/*  now, create points; 6 points per face.*/
	pt = ptr;

	
#define PTF0 (*pt).c[0] =  x; (*pt).c[1] =  y; (*pt).c[2] =  z; pt++;
#define PTF1 (*pt).c[0] = -x; (*pt).c[1] =  y; (*pt).c[2] =  z; pt++;
#define PTF2 (*pt).c[0] = -x; (*pt).c[1] = -y; (*pt).c[2] =  z; pt++;  
#define PTF3 (*pt).c[0] =  x; (*pt).c[1] = -y; (*pt).c[2] =  z; pt++;
#define PTR0 (*pt).c[0] =  x; (*pt).c[1] =  y; (*pt).c[2] =  -z; pt++;
#define PTR1 (*pt).c[0] = -x; (*pt).c[1] =  y; (*pt).c[2] =  -z; pt++;
#define PTR2 (*pt).c[0] = -x; (*pt).c[1] = -y; (*pt).c[2] =  -z; pt++;
#define PTR3 (*pt).c[0] =  x; (*pt).c[1] = -y; (*pt).c[2] =  -z; pt++;


	PTF0 PTF1 PTF2  PTF0 PTF2 PTF3 /* front */
	PTR2 PTR1 PTR0  PTR3 PTR2 PTR0 /* back  */
	PTF0 PTR0 PTR1  PTF0 PTR1 PTF1 /* top   */
	PTF3 PTF2 PTR2  PTF3 PTR2 PTR3 /* bottom */
	PTF0 PTF3 PTR3 	PTF0 PTR3 PTR0 /* right */
	PTF1 PTR1 PTR2  PTF1 PTR2 PTF2 /* left */

	/* finished, and have good data */
	node->__points.p = ptr;
}
#undef PTF0
#undef PTF1
#undef PTF2
#undef PTR0
#undef PTR1
#undef PTR2

void render_Box (struct X3D_Box *node) {
	extern GLfloat boxtex[];		/*  in CFuncs/statics.c*/
	extern GLfloat boxnorms[];		/*  in CFuncs/statics.c*/
	
	struct textureVertexInfo mtf = {boxtex,2,GL_FLOAT,0,NULL};

	float x = ((node->size).c[0])/2;
	float y = ((node->size).c[1])/2;
	float z = ((node->size).c[2])/2;

	/* test for <0 of sides */
	if ((x < 0) || (y < 0) || (z < 0)) return;

	COMPILE_IF_REQUIRED
	if (!node->__points.p) return; /* still compiling */

	/* for BoundingBox calculations */
	setExtent(x,-x,y,-y,z,-z,X3D_NODE(node));

	CULL_FACE(node->solid)

	/*  Draw it; assume VERTEX and NORMALS already defined.*/
	textureDraw_start(&mtf);
	FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,(GLfloat *)node->__points.p);
	FW_GL_NORMAL_POINTER (GL_FLOAT,0,boxnorms);

	/* do the array drawing; sides are simple 0-1-2-3, 4-5-6-7, etc quads */
	FW_GL_DRAWARRAYS (GL_TRIANGLES, 0, 36);
	textureDraw_end();
	gglobal()->Mainloop.trisThisLoop += 24;
}


/*******************************************************************************/

void compile_Cylinder (struct X3D_Cylinder * node) {
	#define CYLDIV 20
	float h = (node->height)/2;
	float r = node->radius;
	int i = 0;
	struct MyVertex cylVert[CYLDIV * 4 * 3];
	int indx = 0;

	/*  have to regen the shape*/
	MARK_NODE_COMPILED


	/* use VBOS for Cylinders? */

		if (node->__cylinderVBO == 0) {
			glGenBuffers(1,(GLuint*) &node->__cylinderVBO);
		}

		/* we create two triangle fans - the cone, and the bottom. */
		/* first, the flat bit on the bottom */
		indx=0;

		if (node->bottom) {
			for (i=0; i<CYLDIV; i++) {
				double angle = PI*2*i/(double)CYLDIV;
				double next_angle = PI*2*(i+1)/(double)CYLDIV;
				/* vertex #1 */
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = -1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(angle); 
				cylVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(angle); 
				indx++;
	
				/* vertex #2 - in the centre */
				cylVert[indx].vert.c[0] = 0.0f;
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = 0.0f;
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = -1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f; 
				cylVert[indx].tc.c[1] = 0.5f; 
				indx++;
	
				/* vertex #3 */
				cylVert[indx].vert.c[0] = r* (float) sin(next_angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(next_angle);
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = -1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(next_angle); 
				cylVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(next_angle); 
				indx++;
			}
		}
		if (node->top) {
			/* same as bottom, but wind'em the other way */
			for (i=0; i<CYLDIV; i++) {
				double angle = PI*2*i/(double)CYLDIV;
				double next_angle = PI*2*(i+1)/(double)CYLDIV;
				/* vertex #1 */
				cylVert[indx].vert.c[0] = r* (float) sin(next_angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(next_angle);
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = 1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(next_angle); 
				cylVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(next_angle); 
				indx++;
	
				/* vertex #2 - in the centre */
				cylVert[indx].vert.c[0] = 0.0f;
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = 0.0f;
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = 1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f; 
				cylVert[indx].tc.c[1] = 0.5f; 
				indx++;
	
				/* vertex #3 */
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				cylVert[indx].norm.c[0] = 0.0f; 
				cylVert[indx].norm.c[1] = 1.0f; 
				cylVert[indx].norm.c[2] = 0.0f;
				cylVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(angle); 
				cylVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(angle); 
				indx++;
			}
		}
	


		/* now, the sides */
		if (node->side) {
			for (i=0; i<CYLDIV; i++) {
				double angle;

				/* Triangle #1 for this segment of the side */
				/* vertex #1 - bottom right */
				angle = (double) (PI * 2 * (i+1.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);

				/* note the angle for normals is half way between faces */
				angle = (double) (PI * 2 * (i+0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle);
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);

				/* note that we use the Cone TC array; assume same division */
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+0];
				cylVert[indx].tc.c[1] = 0.0f; 
				indx++;
	
				/* vertex #2 - top left */
				angle = (double) (PI * 2 * (i+0.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i-0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle); 
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+2];
				cylVert[indx].tc.c[1] = 1.0f; 
				indx++;
	
				/* vertex #3 - bottom left */
				angle = (double) (PI * 2 * (i+0.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i-0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle); 
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+2];
				cylVert[indx].tc.c[1] = 0.0f; 
				indx++;

				/* Triangle #2 for this segment of the side */
				/* vertex #1 - bottom right */
				angle = (double) (PI * 2 * (i+1.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) -h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i+0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle);
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+0];
				cylVert[indx].tc.c[1] = 0.0f;
				indx++;
	
				/* vertex #2 - top right */
				angle = (double) (PI * 2 * (i+1.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i+0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle); 
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+0];
				cylVert[indx].tc.c[1] = 1.0f; 
				indx++;
	
				/* vertex #3 - top left */
				angle = (float) (PI * 2 * (i+0.0f)) / (double) (CYLDIV);
				cylVert[indx].vert.c[0] = r* (float) sin(angle);
				cylVert[indx].vert.c[1] = (float) h;
				cylVert[indx].vert.c[2] = r* (float) cos(angle);
				angle = (double) (PI * 2 * (i-0.5f)) / (double) (CYLDIV);
				cylVert[indx].norm.c[0] = (float) sin(angle); 
				cylVert[indx].norm.c[1] = 0.0f;
				cylVert[indx].norm.c[2] = (float) cos(angle);
				cylVert[indx].tc.c[0] = VBO_coneSideTexParams[i*3+2];
				cylVert[indx].tc.c[1] = 1.0f; 
				indx++;
			}

		}

		node->__cylinderTriangles = indx;

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, node->__cylinderVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(struct MyVertex)*indx, cylVert, GL_STATIC_DRAW);

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
}

void render_Cylinder (struct X3D_Cylinder * node) {
	extern GLfloat cylsidetex[];		/*  in CFuncs/statics.c*/
	struct textureVertexInfo mtf = {cylsidetex,2,GL_FLOAT,0,NULL};    

	float h = (node->height)/2;
	float r = node->radius;


	if ((h < 0) || (r < 0)) {return;}

	/* for BoundingBox calculations */
	setExtent(r,-r,h,-h,r,-r,X3D_NODE(node));

	COMPILE_IF_REQUIRED

	CULL_FACE(node->solid)

		// taken from the OpenGL.org website:
		#define BUFFER_OFFSET(i) ((char *)NULL + (i))

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, node->__cylinderVBO);

		FW_GL_VERTEX_POINTER(3, GL_FLOAT, (GLfloat) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(0));   //The starting point of the VBO, for the vertices
		FW_GL_NORMAL_POINTER(GL_FLOAT, (GLfloat) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(12));   //The starting point of normals, 12 bytes away

		/* set up texture drawing for this guy */
        mtf.pre_canned_textureCoords = NULL;
		mtf.TC_size = 2;
		mtf.TC_type = GL_FLOAT;
		mtf.TC_stride = (GLfloat) sizeof(struct MyVertex);
		mtf.TC_pointer = BUFFER_OFFSET(24);
		textureDraw_start(&mtf);
		/* FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, ConeIndxVBO); */
		FW_GL_DRAWARRAYS(GL_TRIANGLES,0,node->__cylinderTriangles);

		/* turn off */
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);         
	textureDraw_end();
}



/*******************************************************************************/

void compile_Cone (struct X3D_Cone *node) {
	/*  DO NOT change this define, unless you want to recalculate statics below....*/
	#define  CONEDIV 20

	float h = (node->height)/2;
	float r = node->bottomRadius;
	int i;
	struct MyVertex coneVert[CONEDIV * 2 * 3];
	int indx = 0;

	/*  have to regen the shape*/
	MARK_NODE_COMPILED


		if (node->__coneVBO == 0) {
			glGenBuffers(1,(GLuint *) &node->__coneVBO);
		}

		/* we create two triangle fans - the cone, and the bottom. */
		/* first, the flat bit on the bottom */
		indx=0;

		if (node->bottom) {
			for (i=0; i<CONEDIV; i++) {
				double angle = PI*2*i/(double)CONEDIV;
				double next_angle = PI*2*(i+1)/(double)CONEDIV;
				/* vertex #1 */
				coneVert[indx].vert.c[0] = r* (float) sin(angle);
				coneVert[indx].vert.c[1] = (float) -h;
				coneVert[indx].vert.c[2] = r* (float) cos(angle);
				coneVert[indx].norm.c[0] = 0.0f; 
				coneVert[indx].norm.c[1] = -1.0f; 
				coneVert[indx].norm.c[2] = 0.0f;
				coneVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(angle); 
				coneVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(angle); 
				indx++;
	
				/* vertex #2 - in the centre */
				coneVert[indx].vert.c[0] = 0.0f;
				coneVert[indx].vert.c[1] = (float) -h;
				coneVert[indx].vert.c[2] = 0.0f;
				coneVert[indx].norm.c[0] = 0.0f; 
				coneVert[indx].norm.c[1] = -1.0f; 
				coneVert[indx].norm.c[2] = 0.0f;
				coneVert[indx].tc.c[0] = 0.5f; 
				coneVert[indx].tc.c[1] = 0.5f; 
				indx++;
	
				/* vertex #3 */
				coneVert[indx].vert.c[0] = r* (float) sin(next_angle);
				coneVert[indx].vert.c[1] = (float) -h;
				coneVert[indx].vert.c[2] = r* (float) cos(next_angle);
				coneVert[indx].norm.c[0] = 0.0f; 
				coneVert[indx].norm.c[1] = -1.0f; 
				coneVert[indx].norm.c[2] = 0.0f;
				coneVert[indx].tc.c[0] = 0.5f+0.5f* (float) sin(next_angle); 
				coneVert[indx].tc.c[1] = 0.5f+0.5f* (float) cos(next_angle); 
				indx++;
			}
		}


		/* now, the sides */
		if (node->side) {
			GLfloat *tcp =  VBO_coneSideTexParams;

			for (i=0; i<CONEDIV; i++) {
				double angle;

				/* vertex #1 */
				angle = (double) (PI * 2 * (i+1.0f)) / (double) (CONEDIV);
				coneVert[indx].vert.c[0] = r* (float) sin(angle);
				coneVert[indx].vert.c[1] = (float) -h;
				coneVert[indx].vert.c[2] = r* (float) cos(angle);
				coneVert[indx].norm.c[0] = (float) sin(angle);
				coneVert[indx].norm.c[1] = (float)h/(r*2);
				coneVert[indx].norm.c[2] = (float) cos(angle);

				angle = (double) (PI * 2 * (i+0.0f)) / (double) (CONEDIV);
				coneVert[indx].tc.c[0] = *tcp; tcp++;
				coneVert[indx].tc.c[1] = 0.0f; 
				indx++;
	
				/* vertex #2 - in the centre */
				angle = (float) (PI * 2 * (i+0.5f)) / (double)(CONEDIV);
				coneVert[indx].vert.c[0] = 0.0f;
				coneVert[indx].vert.c[1] = (float) h;
				coneVert[indx].vert.c[2] = 0.0f;
				coneVert[indx].norm.c[0] = (float) sin(angle); 
				coneVert[indx].norm.c[1] = (float)h/(r*2);
				coneVert[indx].norm.c[2] = (float) cos(angle);

				coneVert[indx].tc.c[0] = *tcp; tcp++; 
				coneVert[indx].tc.c[1] = 1.0f;
				indx++;
	
				/* vertex #3 */
				angle = (float) (PI * 2 * (i+0.0f)) / (double) (CONEDIV);
				coneVert[indx].vert.c[0] = r* (float) sin(angle);
				coneVert[indx].vert.c[1] = (float) -h;
				coneVert[indx].vert.c[2] = r* (float) cos(angle);
				coneVert[indx].norm.c[0] = (float) sin(angle); 
				coneVert[indx].norm.c[1] = (float)h/(r*2);
				coneVert[indx].norm.c[2] = (float) cos(angle);

				angle = (float) (PI * 2 * (i+1.0f)) / (double) (CONEDIV);
				coneVert[indx].tc.c[0] = *tcp; tcp++; 
				coneVert[indx].tc.c[1] = 0.0f; 
				indx++;
			}

		}

		node->__coneTriangles = indx;

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, (GLuint) node->__coneVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(struct MyVertex)*indx, coneVert, GL_STATIC_DRAW);

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);

		/* no longer needed */
		FREE_IF_NZ(node->__botpoints.p);
		FREE_IF_NZ(node->__sidepoints.p);
		FREE_IF_NZ(node->__normals.p);    
}

void render_Cone (struct X3D_Cone *node) {
    extern float tribottex[];		/*  in CFuncs/statics.c*/
    
	/*  DO NOT change this define, unless you want to recalculate statics below....*/
	#define  CONEDIV 20

	struct textureVertexInfo mtf = {tribottex,2,GL_FLOAT,0,NULL};
				
	float h = (node->height)/2;
	float r = node->bottomRadius;

	if ((h < 0) || (r < 0)) {return;}
	COMPILE_IF_REQUIRED

	/* for BoundingBox calculations */
	setExtent(r,-r,h,-h,r,-r,X3D_NODE(node));


	CULL_FACE(node->solid)
	/*  OK - we have vertex data, so lets just render it.*/
	/*  Always assume GL_VERTEX_ARRAY and GL_NORMAL_ARRAY are enabled.*/
		// taken from the OpenGL.org website:
		#define BUFFER_OFFSET(i) ((char *)NULL + (i))

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, node->__coneVBO);

		FW_GL_VERTEX_POINTER(3, GL_FLOAT, (GLfloat) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(0));   //The starting point of the VBO, for the vertices
		FW_GL_NORMAL_POINTER(GL_FLOAT, (GLfloat) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(12));   //The starting point of normals, 12 bytes away

		/* set up texture drawing for this guy */
        mtf.pre_canned_textureCoords = NULL;
		mtf.TC_size = 2;
		mtf.TC_type = GL_FLOAT;
		mtf.TC_stride = (GLfloat) sizeof(struct MyVertex);
		mtf.TC_pointer = BUFFER_OFFSET(24);
		textureDraw_start(&mtf);
		FW_GL_DRAWARRAYS(GL_TRIANGLES,0,node->__coneTriangles);

		/* turn off */
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);
	textureDraw_end();
}



/*******************************************************************************/
/* how many triangles in this sphere? SPHDIV strips, each strip
	has 2 triangles, and each triangles has 3 vertices and there are
	SPHDIV rows */
#define SPHDIV 20
#define TRISINSPHERE (SPHDIV*3* SPHDIV)


void compile_Sphere (struct X3D_Sphere *node) {
	#define INIT_TRIG1(div) t_aa = (float) sin(PI/(div)); t_aa *= 2*t_aa; t_ab =(float) -sin(2*PI/(div));
	#define START_TRIG1 t_sa = 0; t_ca = -1;
	#define UP_TRIG1 t_sa1 = t_sa; t_sa -= t_sa*t_aa - t_ca * t_ab; t_ca -= t_ca * t_aa + t_sa1 * t_ab;
	#define SIN1 t_sa
	#define COS1 t_ca
	#define INIT_TRIG2(div) t2_aa = (float) sin(PI/(div)); t2_aa *= 2*t2_aa; t2_ab = (float) -sin(2*PI/(div));
	#define START_TRIG2 t2_sa = -1; t2_ca = 0;
	#define UP_TRIG2 t2_sa1 = t2_sa; t2_sa -= t2_sa*t2_aa - t2_ca * t2_ab; t2_ca -= t2_ca * t2_aa + t2_sa1 * t2_ab;
	#define SIN2 t2_sa
	#define COS2 t2_ca

	/*  make the divisions 20; dont change this, because statics.c values*/
	/*  will then need recaculating.*/
	extern GLfloat spherenorms[];		/*  side normals*/
	extern float spheretex[];		/*  in CFuncs/statics.c*/

	int count;
	float rad = node->radius;
	struct SFVec3f *ptr;

	int v; int h;
	float t_aa, t_ab, t_sa, t_ca, t_sa1;
	float t2_aa, t2_ab, t2_sa, t2_ca, t2_sa1;
	struct SFVec3f *pts;

	/*  have to regen the shape*/
	MARK_NODE_COMPILED

	/*  MALLOC memory (if possible)*/
	/*  2 vertexes per points. (+1, to loop around and close structure)*/
	if (!node->__points.p) {
		// malloc points. We seem to never need the ".n" size param, but initialize it
		// anyway to keep things clean and even.
		ptr = MALLOC (struct SFVec3f *,sizeof(struct SFVec3f) * SPHDIV * (SPHDIV+1) * 2);
		node->__points.n = SPHDIV * (SPHDIV+1) * 2;
	} else ptr = node->__points.p;

	pts = ptr;
	count = 0;

	INIT_TRIG1(SPHDIV)
	INIT_TRIG2(SPHDIV)

	START_TRIG1
	{
		extern GLfloat spherenorms[];		/*  side normals*/
		extern float spheretex[];		/*  in CFuncs/statics.c*/

		int myVertexVBOSize = (int) (sizeof(struct SFVec3f) +
                                         sizeof(struct SFVec3f) +
                                         sizeof(struct SFVec2f)) * SPHDIV * (SPHDIV+1) * 2;

		struct MyVertex *SphVBO = MALLOC(struct MyVertex *, myVertexVBOSize);
		struct SFVec3f *myNorms = (struct SFVec3f*)spherenorms;
		struct SFVec2f *myTex = (struct SFVec2f*)spheretex;

		if (node->_sideVBO == 0) {
			glGenBuffers(1,(GLuint *) &node->_sideVBO);
		}
		for(v=0; v<SPHDIV; v++) {
			float vsin1 = SIN1;
			float vcos1 = COS1, vsin2,vcos2;
			UP_TRIG1
			vsin2 = SIN1;
			vcos2 = COS1;
			START_TRIG2
			for(h=0; h<=SPHDIV; h++) {
				float hsin1 = SIN2;
				float hcos1 = COS2;
				UP_TRIG2
				pts[count].c[0] = rad * vsin2 * hcos1;
				pts[count].c[1] = rad * vcos2;
				pts[count].c[2] = rad * vsin2 * hsin1;
				
				/* copy these points into the MyVertex Sphere VBO */
				memcpy (SphVBO[count].vert.c, pts[count].c, sizeof (struct SFVec3f));
				memcpy (SphVBO[count].norm.c, myNorms[count].c, sizeof (struct SFVec3f));
				memcpy (SphVBO[count].tc.c, myTex[count].c, sizeof (struct SFVec2f));
				count++;
				pts[count].c[0] = rad * vsin1 * hcos1;
				pts[count].c[1] = rad * vcos1;
				pts[count].c[2] = rad * vsin1 * hsin1;
				/* copy these points into the MyVertex Sphere VBO */
				memcpy (SphVBO[count].vert.c, pts[count].c, sizeof (struct SFVec3f));
				memcpy (SphVBO[count].norm.c, myNorms[count].c, sizeof (struct SFVec3f));
				memcpy (SphVBO[count].tc.c, myTex[count].c, sizeof (struct SFVec2f));
				count++;
			}
		}	

 		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, (GLuint) node->_sideVBO);
		glBufferData(GL_ARRAY_BUFFER, myVertexVBOSize, SphVBO, GL_STATIC_DRAW);

		if (node->__SphereIndxVBO == 0) {
			ushort pindices[TRISINSPHERE*2];
			ushort *pind = pindices;
			int row;
			int indx;

			glGenBuffers(1,(GLuint *)&node->__SphereIndxVBO);
			//for (count=0; count<TRISINSPHERE*2; count++) pindices[count]=0;
			for (row=0; row<SPHDIV; row++) {
				indx=42*row;
				for (count = 0; count < SPHDIV*2; count+=2) {
					*pind = indx; pind++;
					*pind = indx+1; pind++;
					*pind = indx+2; pind++;
					//printf ("wrote %u %u %u\n",indx, indx+1, indx+2);
					*pind = indx+2; pind++;
					*pind = indx+1; pind++;
					*pind = indx+3; pind++;
					//printf ("wrote %u %u %u\n",indx+2, indx+1, indx+3);
					indx+=2;
				}
			}
 			FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, node->__SphereIndxVBO);
 			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ushort)*TRISINSPHERE*2, pindices, GL_STATIC_DRAW);
		}


		FREE_IF_NZ(SphVBO);
                FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
	}
	/* finished - for threading */
	node->__points.p = ptr;
}



void render_Sphere (struct X3D_Sphere *node) {
	/*  make the divisions 20; dont change this, because statics.c values*/
	/*  will then need recaculating.*/
	
	extern GLfloat spherenorms[];		/*  side normals*/
	extern float spheretex[];		/*  in CFuncs/statics.c*/

	struct textureVertexInfo mtf = {spheretex,2,GL_FLOAT,0,NULL};


	float rad = node->radius;

	if (rad<=0.0) { return;}

	/* for BoundingBox calculations */
	setExtent(rad,-rad,rad,-rad,rad,-rad,X3D_NODE(node));

	COMPILE_IF_REQUIRED

	CULL_FACE(node->solid)

	/*  Display the shape*/

		// taken from the OpenGL.org website:
		#define BUFFER_OFFSET(i) ((char *)NULL + (i))

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, (GLuint) node->_sideVBO);

		FW_GL_VERTEX_POINTER(3, GL_FLOAT, (GLfloat) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(0));   //The starting point of the VBO, for the vertices
		FW_GL_NORMAL_POINTER(GL_FLOAT, (GLfloat)  sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(12));   //The starting point of normals, 12 bytes away
                mtf.pre_canned_textureCoords = NULL;
                mtf.TC_size = 2;
                mtf.TC_type = GL_FLOAT;
                mtf.TC_stride = (GLfloat) sizeof(struct MyVertex);
                mtf.TC_pointer = BUFFER_OFFSET(24);
		textureDraw_start(&mtf);

		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, node->__SphereIndxVBO);
		
		FW_GL_DRAWELEMENTS (GL_TRIANGLES, TRISINSPHERE, GL_UNSIGNED_SHORT, (int *)BUFFER_OFFSET(0));   //The starting point of the IBO

		/* turn off */
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);

	textureDraw_end();

///* DJTRACK_PICKSENSORS */
//#ifdef DJTRACK_PICKSENSORS
//	if(active_picksensors()) {
//		struct X3D_Node *chained ;
//		int pickable = FALSE ;
//		chained = (struct X3D_Node *) node ;
//
//		node->_renderFlags = node->_renderFlags & (0xFFFF^VF_inPickableGroup);
//		chainUpPickableTree((struct X3D_Node *) node, chained, -1) ;
//		pickable = (node->_renderFlags & VF_inPickableGroup) ;
//
//		if(pickable) {
//			rewind_picksensors();
//			/* dump_scene(stdout, 0, (struct X3D_Node *)rootNode); */
//			//dump_scene(stdout, 0, (struct X3D_Node *)node);
//			while (more_picksensors() == TRUE) {
//				void * XX = get_picksensor();
//				printf("%s,%d render_Sphere %p test PickSensor\n",__FILE__,__LINE__,XX);
//				//dump_scene (stdout, 0, XX) ;
//				advance_picksensors();
//			}
//		}
//	}
//#endif
}

void render_IndexedFaceSet (struct X3D_IndexedFaceSet *node) {
		COMPILE_POLY_IF_REQUIRED (node->coord, node->color, node->normal, node->texCoord)
		if (!node->_intern) return;
		CULL_FACE(node->solid)
		render_polyrep(node);
}

void render_ElevationGrid (struct X3D_ElevationGrid *node) {
		COMPILE_POLY_IF_REQUIRED (NULL, node->color, node->normal, node->texCoord)
		CULL_FACE(node->solid)
		render_polyrep(node);
}

void render_Extrusion (struct X3D_Extrusion *node) {
		COMPILE_POLY_IF_REQUIRED (NULL,NULL,NULL,NULL)
		CULL_FACE(node->solid)
		render_polyrep(node);
}

/* Fast culling possibilities:
   a) sphere-sphere
   b) MBB (axes-aligned Minimum Bounding Box or 'extent') (pr->maxVals[i], pr->minVals[i] i=0,1,2 or equivalent)
   two optional spaces in which to do the comparison: shape space or avatar(collision) space
   1. shape sphere/MBB > [Shape2Collision] > collision space coordinates. Or - 
   2. avatar collision volume > [Collision2Shape] > into shape coords
   which ever way, we expect false positives, but don't want any false negatives which could make you fall through 
   terrain or slide through walls. To be safe, error on the side of bigger collision volumes for shape and avatar.
*/

/* note as of Jan 16, 2010 - not all collide_<shape> scenarios come through the following avatarCollisionVolumeIntersectMBB. 
   All walking scnearios do. 
   collide_generic, collide_box, collide_extrusion, collide_Text, collide_Rectangle2D: all walk/fly/examine do. 
   collide_sphere,collide_cylinder,collide_cone - only walk comes through here. fly/examine done with the original analytical shape/analytical avatar code.
*/

int avatarCollisionVolumeIntersectMBB(double *modelMatrix, double *prminvals, double* prmaxvals) 
{
	/* returns 1 if your shape MBB overlaps the avatar collision volume
	   modelMatrix[16] == shape2collision transform. collision space is like avatar space, except vertical aligned to gravity - current bound viewpoint(walk), or avatar(fly/examine) or global gravity
	   prminvals[3],prmaxvals[3] - MBB minimum bounding box or extent of shape, in shape space
	   the fastTestMethod can be set in mainloop.c render_collisions()
	*/
	ttglobal tg = gglobal();
	struct sFallInfo* fi = FallInfo();
	if(fi->walking)
	{
		/* cylindrical / popcycle shaped avatar collision volume */
		GLDOUBLE awidth = tg->Bindable.naviinfo.width; /*avatar width*/
		GLDOUBLE atop = tg->Bindable.naviinfo.width; /*top of avatar (relative to eyepoint)*/
		GLDOUBLE abottom = -tg->Bindable.naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
		GLDOUBLE astep = -tg->Bindable.naviinfo.height+tg->Bindable.naviinfo.step;

		/* the following 2 flags are checked a few levels down, in the triangle/quad intersect avatar code get_poly_disp_2(p, 3, nused) */
		fi->checkCylinder = 1; /* 1= shape MBB overlaps avatar collision MBB, else 0 */
		fi->checkFall = 1;     /* 1= shape MBB overlaps avatar fall/climb line segment else 0 */
		fi->checkPenetration = 1;

		{
		   /*  minimum bounding box MBB test in avatar/collision space */
			double foot = abottom;
			if(fi->allowClimbing) foot = astep; /* < popcycle shaped avatar collision volume */
			/* do fall/climb bounds test (popcycle stick) */
			fi->checkFall = fi->canFall; /* only do the falling/climbing if we aren't already colliding - colliding over-rules falling/climbing */
			if(fi->checkFall) fi->checkFall = fast_ycylinder_MBB_intersect_collisionSpace(-fi->fallHeight,atop,0.0, modelMatrix, prminvals, prmaxvals);
			/* do collision volume bounds test (popcycle succulent part)*/
			fi->checkCylinder = fast_ycylinder_MBB_intersect_collisionSpace(foot,atop,awidth, modelMatrix, prminvals, prmaxvals);
			fi->checkPenetration = 0;
			if( fi->canPenetrate )
				fi->checkPenetration = overlapMBBs(fi->penMin,fi->penMax,prminvals,prmaxvals);
			if(!fi->checkCylinder && !fi->checkFall && !fi->checkPenetration){/*printf("$");*/ return 0;} 
		}

	}
	else
	{
		/* examine/fly spherical avatar collision volume */
		GLDOUBLE awidth = tg->Bindable.naviinfo.width; /*avatar width - used as avatar sphere radius*/


			if( !fast_sphere_MBB_intersect_collisionSpace(awidth, modelMatrix, prminvals, prmaxvals )) return 0;

	}
	return 1;
}

int avatarCollisionVolumeIntersectMBBf(double *modelMatrix, float *minVals, float *maxVals)
{
	/* converts pr.floats to doubles and re-delegates */
	int i;
	GLDOUBLE prminvals[3],prmaxvals[3];
	for(i=0;i<3;i++)
	{
		prminvals[i] = minVals[i]; 
		prmaxvals[i] = maxVals[i];
	}
	return avatarCollisionVolumeIntersectMBB(modelMatrix, prminvals,prmaxvals);
}

void collide_genericfaceset (struct X3D_IndexedFaceSet *node ){
	       GLDOUBLE modelMatrix[16];
	       struct point_XYZ delta = {0,0,0};

		#ifdef RENDERVERBOSE
		struct point_XYZ t_orig = {0,0,0};
		#endif


	       struct X3D_PolyRep pr;
	       prflags flags = 0;
	       int change = 0;

		/* JAS - first pass, intern is probably zero */
		if (((struct X3D_PolyRep *)node->_intern) == 0) return;

		/* JAS - no triangles in this text structure */
		if ((((struct X3D_PolyRep *)node->_intern)->ntri) == 0) return;

	       /*save changed state.*/
	       if(node->_intern) change = ((struct X3D_PolyRep *)node->_intern)->irep_change;
		COMPILE_POLY_IF_REQUIRED (NULL, NULL, NULL, NULL)


	       if(node->_intern) ((struct X3D_PolyRep *)node->_intern)->irep_change = change;
	       /*restore changes state, invalidates mk_polyrep work done, so it can be done
	         correclty in the RENDER pass */

	       if(!node->solid) {
		   flags = flags | PR_DOUBLESIDED;
	       }

	       pr = *((struct X3D_PolyRep*)node->_intern);


		/* IndexedFaceSets are "different", in that the user specifies points, among
		   other things.  The rendering pass takes these external points, and streams
		   them to make rendering much faster on hardware accel. We have to check to
		   see whether we have got here before the first rendering of a possibly new
		   IndexedFaceSet */
		if (!pr.actualCoord) {
			struct Multi_Vec3f* tmp;
			tmp = getCoordinate(node->coord,"Collision");
			pr.actualCoord = (float *) tmp->p;
		}

	       FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);
		   /* 
			For examine and fly navigation modes, there's no gravity direction. 
			Avatar collision volume is aligned to avatar and is spherical and 
			symmetrically directional. This is the simple case. Specifications say for 
			walk navigation mode gravity vector is down {0,-1,0} with respect to (wrt) the 
			currently bound viewpoint, not including the viewpoint's orientation 
			field and not including avatar navigation/tilt away from its parent 
			bound-viewpoint pose. When you collide in walk mode, the avatar collision
			volume is aligned to bound-viewpoint. This is a slightly more complex case.
			To generalize the 2 cases, some	Definitions:
			Spaces:
				Avatar space 
					- gravity is avatar-down
					- Avatar is at {0,0,0} and +Y up is as you see up in your current view, 
					  +Z aft and +X to the right
				BoundViewpoint space - currently bound viewpoint (transform parent to avatar)
				BVVA space - Bound-Viewpoint-Vertically-aligned Avatar-centric 
					- same as Avatar space with avatar at {0,0,0} except tilted so that gravity 
					  is in the direction of down {0,-1,0} for the currently bound viewpoint 
					  (instead for avatar), as per specs
					- gravity is bound-viewpoint down
				Collision space - Fly/Examine mode: Avatar space. Walk mode: BVVA space
					- the avatar collision volume - height, stepsize, width - are defined for 
					  collision space and axes aligned with it
				Shape space - raw shape <Coordinate> space
			Transforms:
				Bound2Avatar     - transforms from BoundViewpoint space to Avatar space 
				                 - computed from viewer.quat,viewer.pos
				Avatar2BVVA,BVVA2Avatar 
								- computed from downvector in BoundViewpoint space transformed
								   via Bound2Avatar into Avatar space
								- constant for a frame, computable once navigation mode and 
								   avatar pose is know for the frame
				Avatar2Collision - Fly/Examine: Identity,    Walk: Avatar2BVVA
				Collision2Avatar - Fly/Examine: Identity,    Walk: BVVA2Avatar
				Shape2Collision  - Fly/Examine: modelMatrix, Walk: Avatar2BVVA*modelMatrix

			goal: transform shape geometry into Collision space. (The avatar collision 
			         volume is by definition in collision space.)
			      Do some collisions between shape and avatar.
				  Transform collision correction deltas from collision space to avatar space, 
				      apply deltas to avatar position.
		    implementation:
				transform shape into collision space - Fly/Examine:modelMatrix 
				        or Walk:(Avatar2BVVA * modelMatrix)
				transform collision correction deltas from collision space to avatar space: 
					- done in Mainloop.c get_collisionoffset() with FallInfo.collision2avatar
		   */

			matmultiply(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
			//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 


			#ifdef RENDERVERBOSE
                           t_orig.x = modelMatrix[12];
                           t_orig.y = modelMatrix[13];
                           t_orig.z = modelMatrix[14];
			#endif

		   /* at this point, whichever method - modelMatrix is Shape2Collision 
		         and upvecmat is Collision2Avatar 
		   - pr.actualCoord - these are Shape space coordinates
			They will be transformed into CollisionSpace coordinates by the modelMatrix transform.
		   */
			if(!avatarCollisionVolumeIntersectMBBf(modelMatrix, pr.minVals, pr.maxVals))return;
		   /* passed fast test. Now for gruelling test */
			delta = polyrep_disp2(pr,modelMatrix,flags); //polyrep_disp(abottom,atop,astep,awidth,pr,modelMatrix,flags);
		   /* delta is in collision space */
			/* lets say you are floating above ground by 3 units + avatar.height 1.75 = 4.75. 
			Then delta = (0,3,0)*/
	       vecscale(&delta,&delta,-1);
	       accumulate_disp(CollisionInfo(),delta); /* we are accumulating in collision space (fly/examine: avatar space, walk: BVVA space) */

		#ifdef RENDERVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
/*		   printmatrix(modelMatrix);*/
		   fprintf(stderr,"COLLISION_IFS: (%f %f %f) (%f %f %f)\n",
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z
			  );

	       }
		#endif

}

typedef int cquad[4];
typedef int ctri[3];
struct sCollisionGeometry
{
	struct point_XYZ *pts;
	struct point_XYZ *tpts;
	ctri *tris;
	int ntris;
	cquad *quads;
	int nquads;
	int npts;
	double smin[3],smax[3];
};

static struct sCollisionGeometry collisionSphere = {NULL,NULL,NULL,0,NULL,0,0, {0.0,0.0,0.0},{0.0,0.0,0.0}};
static struct sCollisionGeometry collisionCylinder = {NULL,NULL,NULL,0,NULL,0,0, {0.0,0.0,0.0},{0.0,0.0,0.0}};
static struct sCollisionGeometry collisionCone = {NULL,NULL,NULL,0,NULL,0,0, {0.0,0.0,0.0},{0.0,0.0,0.0}};

static void collisionSphere_init(struct X3D_Sphere *node)
{
	int i,j,count;
	/* for debug int k, biggestNum; */
	double radinverse;
	struct SFVec3f *pts = node->__points.p;

	/*  re-using the compile_sphere node->__points data which is organized into GL_QUAD_STRIPS
		my understanding: there are SPHDIV/2 quad strips. Each quadstrip has SPHDIV quads, and enough points to do that many quads
		without sharing points with other quadstrips. So to make SPHDIV quads, you need 2 rows of SPHDIV+1 points.
		because there's 2 triangles per quad, there should be 2x as many triangles as quads.
		num quads = SPHDIV/2 x SPHDIV
		num points = SPHDIV/2 X [(SPHDIV+1) X 2] = SPHDIV*(SPHDIV+1)
		num tris = quads x 2 = SPHDIV X SPHDIV 
	*/

	collisionSphere.npts = SPHDIV*(SPHDIV+1);
	collisionSphere.pts = malloc(collisionSphere.npts * sizeof(struct point_XYZ));
	collisionSphere.tpts = malloc(collisionSphere.npts * sizeof(struct point_XYZ));
	/* undo radius field on node so radius == 1 (generic, for all spheres, scale back later) */
	radinverse = 1.0;
	if( !APPROX(node->radius,0.0) ) radinverse = 1.0/node->radius;
	for(i=0;i<collisionSphere.npts;i++)
	{
		collisionSphere.pts[i].x = pts[i].c[0] * radinverse;
		collisionSphere.pts[i].y = pts[i].c[1] * radinverse;
		collisionSphere.pts[i].z = pts[i].c[2] * radinverse;
	}


	collisionSphere.ntris = SPHDIV * SPHDIV;
	collisionSphere.tris = malloc(collisionSphere.ntris * sizeof(ctri));
	collisionSphere.nquads = 0;
	count = 0;
	for(i = 0; i < SPHDIV/2; i ++)  
	{ 
		/* one quad strip  of  SPHDIV quads or SPHDIV*2 triangles */
		for(j=0;j<(2*SPHDIV);j+=2) //=+)
		{
			/* first triangle */
			collisionSphere.tris[count][0] = i*(SPHDIV+1)*2 + j;
			collisionSphere.tris[count][1] = i*(SPHDIV+1)*2 + j+1;
			collisionSphere.tris[count][2] = i*(SPHDIV+1)*2 + j+2; 
			count ++;
			/* second triangle */
			collisionSphere.tris[count][0] = i*(SPHDIV+1)*2 + j+3; 
			collisionSphere.tris[count][1] = i*(SPHDIV+1)*2 + j+2; 
			collisionSphere.tris[count][2] = i*(SPHDIV+1)*2 + j+1; 
			count ++;
		}
	}
	/* count should == num triangles 
	debug check on indexing - biggestNum should == npts -1
	biggestNum = 0;
	for(i=0;i<collisionSphere.ntris;i++)
		for(j=0;j<3;j++)
			biggestNum = max(biggestNum,collisionSphere.tris[i][j]);
	*/
	/* MBB */
	for(i=0;i<3;i++)
	{
		collisionSphere.smin[i] = -1.0; //rad;
		collisionSphere.smax[i] =  1.0; //rad;
	}

}
#ifdef DEBUGGING_CODE
DEBUGGING_CODEint collisionSphere_render(double radius)
DEBUGGING_CODE{
DEBUGGING_CODE	/* I needed to verify the collision mesh sphere was good, and it uses triangles, so I drew it the triangle way and it looked good 
DEBUGGING_CODE	   to see it draw, you need to turn on collision and get close to a sphere - then it will initialize and start drawing it.
DEBUGGING_CODE	*/
DEBUGGING_CODE	int i,j,count,highest;
DEBUGGING_CODE	count = 0;
DEBUGGING_CODE	highest = 0;
DEBUGGING_CODE	for(i =0; i < collisionSphere.ntris; i++)  
DEBUGGING_CODE	{ 
DEBUGGING_CODE		struct point_XYZ pts[3]; //,a,b,n;
DEBUGGING_CODE		pts[0] = collisionSphere.pts[collisionSphere.tris[i][0]];
DEBUGGING_CODE		pts[1] = collisionSphere.pts[collisionSphere.tris[i][1]];
DEBUGGING_CODE		pts[2] = collisionSphere.pts[collisionSphere.tris[i][2]];
DEBUGGING_CODE		FW_GL_BEGIN(GL_TRIANGLES);
DEBUGGING_CODE		for(j=0;j<3;j++)
DEBUGGING_CODE			FW_GL_VERTEX3D(pts[j].x*radius,pts[j].y*radius,pts[j].z*radius);
DEBUGGING_CODE#define FW_GL_END() glEnd()
DEBUGGING_CODE		FW_GL_END();
DEBUGGING_CODE	}
DEBUGGING_CODE	return 0;
DEBUGGING_CODE}
#endif

struct point_XYZ get_poly_disp_2(struct point_XYZ* p, int num, struct point_XYZ n);
#define FLOAT_TOLERANCE 0.00000001
void collide_Sphere (struct X3D_Sphere *node) {
	       struct point_XYZ t_orig = {0,0,0}; /*transformed origin*/
	       struct point_XYZ p_orig= {0,0,0} ; /*projected transformed origin */
	       struct point_XYZ n_orig = {0,0,0}; /*normal(unit length) transformed origin */
	       GLDOUBLE modelMatrix[16];
	       GLDOUBLE dist2;
	       struct point_XYZ delta = {0,0,0};
	       GLDOUBLE radius;
			ttglobal tg = gglobal();
	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLDOUBLE awidth = tg->Bindable.naviinfo.width; /*avatar width*/
	       GLDOUBLE atop = tg->Bindable.naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLDOUBLE abottom = -tg->Bindable.naviinfo.height; /*bottom of avatar (relative to eyepoint)*/

		/* this sucker initialized yet? */
		if (node->__points.p == NULL) return;


	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

		/* apply radius to generic r=1 sphere */
		//radscale.x = radscale.y = radscale.z = node->radius;
		//scale_to_matrix (modelMatrix, &radscale);
		//matmultiply(modelMatrix,FallInfo.avatar2collision,modelMatrix); 

		if(FallInfo()->walking)
		{
			/* mesh method */

			int i;
			double disp;
			struct point_XYZ n;
			struct point_XYZ a,b, dispv, maxdispv = {0,0,0};
			struct point_XYZ radscale;
			double maxdisp = 0;
			radscale.x = radscale.y = radscale.z = node->radius;
			scale_to_matrix (modelMatrix, &radscale);
			matmultiply(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
			//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 

			if(!collisionSphere.npts) collisionSphere_init(node);
			if( !avatarCollisionVolumeIntersectMBB(modelMatrix, collisionSphere.smin,collisionSphere.smax)) return;

			for(i=0;i<collisionSphere.npts;i++)
				transform(&collisionSphere.tpts[i],&collisionSphere.pts[i],modelMatrix);

			for(i = 0; i < collisionSphere.ntris; i++) 
			{
				/*only clip faces "facing" origin */
				//if(vecdot(&n[ci],&middle) < 0.) 
				{
					struct point_XYZ pts[3];
					pts[0] = collisionSphere.tpts[collisionSphere.tris[i][0]];
					pts[1] = collisionSphere.tpts[collisionSphere.tris[i][1]];
					pts[2] = collisionSphere.tpts[collisionSphere.tris[i][2]];
					/* compute normal - could compute once in shapespace then transform */
					VECDIFF(pts[1],pts[0],a);
					VECDIFF(pts[2],pts[1],b); /* or [2] [0] direction not sensitive for some functions */
					veccross(&n,a,b); /* 6 multiplies */
					vecnormal(&n,&n); 
					dispv = get_poly_disp_2(pts,3,n);
					disp = vecdot(&dispv,&dispv);
					if( (disp > FLOAT_TOLERANCE) && (disp > maxdisp) ){
						maxdisp = disp;
						maxdispv = dispv;
					}
				}
			}
			delta = maxdispv;
		        vecscale(&delta,&delta,-1);
		}
		else
		{
			/* easy analytical sphere-sphere stuff */
			matmultiply(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
			//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 

			t_orig.x = modelMatrix[12];
			t_orig.y = modelMatrix[13];
			t_orig.z = modelMatrix[14];
			radius = pow(det3x3(modelMatrix),1./3.) * node->radius;

			/* squared distance to center of sphere (on the y plane)*/
			dist2 = t_orig.x * t_orig.x + t_orig.z * t_orig.z;

			/* easy tests. clip as if sphere was a box */
			/*clip with cylinder */

			if(dist2 - (radius + awidth) * (radius +awidth) > 0) {
				return;
			}
			/*clip with bottom plane */
			if(t_orig.y + radius < abottom) {
				return;
			}
			/*clip with top plane */
			if(t_orig.y-radius > atop) {
				return;
			}

			/* project onto (y x t_orig) plane */
			p_orig.x = sqrt(dist2);
			p_orig.y = t_orig.y;
			p_orig.z = 0;
			/* we need this to unproject rapidly */
			/* n_orig is t_orig.y projected on the y plane, then normalized. */
			n_orig.x = t_orig.x;
			n_orig.y = 0.0;
			n_orig.z = t_orig.z;
			VECSCALE(n_orig,1.0/p_orig.x); /*equivalent to vecnormal(n_orig);, but faster */
			#ifdef RENDERVERBOSE
			printf ("sphere, p_orig %lf %lf %lf, n_orig %lf %lf %lf\n",p_orig.x, p_orig.y, p_orig.z, n_orig.x, n_orig.y, n_orig.z);
			#endif

			   /* 5 cases : sphere is over, over side, side, under and side, under (relative to y axis) */
			   /* these 5 cases correspond to the 5 vornoi regions of the cylinder */
			   if(p_orig.y > atop) {

			   if(p_orig.x < awidth) {
				#ifdef RENDERVERBOSE
				   printf(" /* over, we push down. */ \n");
				#endif

				   delta.y = (p_orig.y - radius) - (atop);
			   } else {
				   struct point_XYZ d2s;
		                   GLDOUBLE ratio;
				   #ifdef RENDERVERBOSE
				printf(" /* over side */ \n");
				#endif

				   /* distance vector from corner to center of sphere*/
				   d2s.x = p_orig.x - awidth;
				   d2s.y = p_orig.y - (atop);
				   d2s.z = 0;

				   ratio = 1- radius/sqrt(d2s.x * d2s.x + d2s.y * d2s.y);

				   if(ratio >= 0) {
				   /* no collision */
				   return;
				   }

				   /* distance vector from corner to surface of sphere, (do the math) */
				   VECSCALE(d2s, ratio );

				   /* unproject, this is the fastest way */
				   delta.y = d2s.y;
				   delta.x = d2s.x* n_orig.x;
				   delta.z = d2s.x* n_orig.z;
			   }
			   } else if(p_orig.y < abottom) {
			   if(p_orig.x < awidth) {
				#ifdef RENDERVERBOSE
				   printf(" /* under, we push up. */ \n");
				#endif

				   delta.y = (p_orig.y + radius) -abottom;
			   } else {
				   struct point_XYZ d2s;
		                 GLDOUBLE ratio;
				#ifdef RENDERVERBOSE
				   printf(" /* under side */ \n");
				#endif

				   /* distance vector from corner to center of sphere*/
				   d2s.x = p_orig.x - awidth;
				   d2s.y = p_orig.y - abottom;
				   d2s.z = 0;

				   ratio = 1- radius/sqrt(d2s.x * d2s.x + d2s.y * d2s.y);

				   if(ratio >= 0) {
				   /* no collision */
				   return;
				   }

				   /* distance vector from corner to surface of sphere, (do the math) */
				   VECSCALE(d2s, ratio );

				   /* unproject, this is the fastest way */
				   delta.y = d2s.y;
				   delta.x = d2s.x* n_orig.x;
				   delta.z = d2s.x* n_orig.z;
			   }

			   } else {
			   #ifdef RENDERVERBOSE
				printf(" /* side */ \n");
			   #endif

			   /* push to side */
			   delta.x = ((p_orig.x - radius)- awidth) * n_orig.x;
			   delta.z = ((p_orig.x - radius)- awidth) * n_orig.z;
			   }

			}
	       accumulate_disp(CollisionInfo(),delta);

		#ifdef RENDERVERBOSE
	       if((delta.x != 0. || delta.y != 0. || delta.z != 0.))
	           printf("COLLISION_SPH: (%f %f %f) (%f %f %f) (px=%f nx=%f nz=%f)\n",
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z,
			  p_orig.x, n_orig.x, n_orig.z
			  );
		#endif
}


void collide_Box (struct X3D_Box *node) {
	       /*easy access, naviinfo.step unused for sphere collisions */
			ttglobal tg = gglobal();
	       GLDOUBLE awidth = tg->Bindable.naviinfo.width; /*avatar width*/
	       GLDOUBLE atop = tg->Bindable.naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLDOUBLE abottom = -tg->Bindable.naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLDOUBLE astep = -tg->Bindable.naviinfo.height+tg->Bindable.naviinfo.step;

	       GLDOUBLE modelMatrix[16];
	       struct point_XYZ iv = {0,0,0};
	       struct point_XYZ jv = {0,0,0};
	       struct point_XYZ kv = {0,0,0};
	       struct point_XYZ ov = {0,0,0};

	       struct point_XYZ delta;


                iv.x = ((node->size).c[0]);
                jv.y = ((node->size).c[1]);
                kv.z = ((node->size).c[2]);
                ov.x = -((node->size).c[0])/2; ov.y = -((node->size).c[1])/2; ov.z = -((node->size).c[2])/2;


	       /* get the transformed position of the Box, and the scale-corrected radius. */
	       FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);
			matmultiply(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
			//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 
			{
				int i;
				double shapeMBBmin[3],shapeMBBmax[3];
				for(i=0;i<3;i++)
				{
					shapeMBBmin[i] = DOUBLE_MIN(-(node->size).c[i]*.5,(node->size).c[i]*.5);
					shapeMBBmax[i] = DOUBLE_MAX(-(node->size).c[i]*.5,(node->size).c[i]*.5);
				}

				if( !avatarCollisionVolumeIntersectMBB(modelMatrix, shapeMBBmin,shapeMBBmax)) return;
			}
	       /* get transformed box edges and position */
	       transform(&ov,&ov,modelMatrix);
	       transform3x3(&iv,&iv,modelMatrix);
	       transform3x3(&jv,&jv,modelMatrix);
	       transform3x3(&kv,&kv,modelMatrix);


	       delta = box_disp(abottom,atop,astep,awidth,ov,iv,jv,kv);

	       vecscale(&delta,&delta,-1);

	       accumulate_disp(CollisionInfo(),delta);

		#ifdef RENDERVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_BOX: (%f %f %f) (%f %f %f)\n",
			  ov.x, ov.y, ov.z,
			  delta.x, delta.y, delta.z
			  );
	       if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) kv=(%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  kv.x, kv.y, kv.z
			  );
		#endif
}


#define  CONEDIV 20

static void collisionCone_init(struct X3D_Cone *node)
{
	/* for debug ctri ct; */
	/* for debug struct point_XYZ a,b,n; */
	int i,count;
	/* for debug int j,k,biggestNum; */
	double h,r,inverseh,inverser;
	struct SFVec3f *pts;// = node->__botpoints;
	struct SFVec3f *pt;
	struct Multi_Vec3f botpoints;
    	
	/*  re-using the compile_cone node->__points data which is organized into GL_TRAIANGLE_FAN (bottom) and GL_TRIANGLES (side)

		my understanding: 
		bottom: there are CONEDIV triangles arranged in a fan around a center point, with CONEDIV perimeter points 
		side: there are CONEDIV side triangles formed with the top point and perimeter points
		num triangles: CONEDIV x 2
		num points: CONEDIV perimeter + center bottom + centre top = CONEDIV+2 
		__botpoints:
			pt[0] - top point of cone
			pt[1-CONEDIV] - perimeter points
			pt[CONEDIV+1] - centre of bottom
		(there are sidepoints too, but duplicate the points in botpoints) 
	*/
	collisionCone.npts = CONEDIV+2;
	collisionCone.pts = malloc(collisionCone.npts * sizeof(struct point_XYZ));
	collisionCone.tpts = malloc(collisionCone.npts * sizeof(struct point_XYZ));

	collisionCone.ntris = CONEDIV *2;
	collisionCone.tris = malloc(collisionCone.ntris * sizeof(ctri));
	count = 0;
	h = (node->height); ///2;
	r = node->bottomRadius;
	inverseh = 1.0;
	inverser = 1.0;
	if( !APPROX(h,0.0) ) inverseh = 1.0/h;
	if( !APPROX(r,0.0) ) inverser = 1.0/r;

    
		/* ok - we copy the non-VBO code here so that Doug Sandens Cylinder Collision code
		   uses the same algorithm whether running in VBO mode or not */

		/*  MALLOC memory (if possible)*/
		botpoints.p = MALLOC (struct SFVec3f *, sizeof(struct SFVec3f)*(CONEDIV+3));

		/*  generate the vertexes for the triangles; top point first. (note: top point no longer used)*/
		pt = botpoints.p;
		pt[0].c[0] = 0.0f; pt[0].c[1] = (float) h; pt[0].c[2] = 0.0f;
		for (i=1; i<=CONEDIV; i++) {
			pt[i].c[0] = (float) (r* sin(PI*2*i/(float)CONEDIV));
			pt[i].c[1] = (float) -h;
			pt[i].c[2] = (float) (r* cos(PI*2*i/(float)CONEDIV));
		}
		/*  and throw another point that is centre of bottom*/
		pt[CONEDIV+1].c[0] = 0.0f; pt[CONEDIV+1].c[1] = (float) -h; pt[CONEDIV+1].c[2] = 0.0f;

		/*  and, for the bottom, [CONEDIV] = [CONEDIV+2]; but different texture coords, so...*/
		memcpy (&pt[CONEDIV+2].c[0],&pt[CONEDIV].c[0],sizeof (struct SFVec3f));

#ifdef fwIGNORE
		/*  side triangles. Make 3 seperate points per triangle... makes FW_GL_DRAWARRAYS with normals*/
		/*  easier to handle.*/
		/*  rearrange bottom points into this array; top, bottom, left.*/
		spt = sidepoints.p;
		for (i=0; i<CONEDIV; i++) {
			/*  top point*/
			spt[i*3].c[0] = 0.0f; spt[i*3].c[1] = (float) h; spt[i*3].c[2] = 0.0f;
			/*  left point*/
			memcpy (&spt[i*3+1].c[0],&pt[i+1].c[0],sizeof (struct SFVec3f));
			/* right point*/
			memcpy (&spt[i*3+2].c[0],&pt[i+2].c[0],sizeof (struct SFVec3f));
		}

		/*  wrap bottom point around once again... ie, final right point = initial left point*/
		memcpy (&spt[(CONEDIV-1)*3+2].c[0],&pt[1].c[0],sizeof (struct SFVec3f));
#endif
	


		pts = botpoints.p;
		for(i=0;i<(CONEDIV+2);i++)
		{
			/* points */
			collisionCone.pts[i].x = pts[i].c[0]*inverser;
			collisionCone.pts[i].y = pts[i].c[1]*inverseh;
			collisionCone.pts[i].z = pts[i].c[2]*inverser;
		}
		for(i=0;i<CONEDIV;i++)
		{
			/* side triangles */
			collisionCone.tris[count][0] = 0;   /* top point */
			collisionCone.tris[count][1] = i +1;
			collisionCone.tris[count][2] = i > (CONEDIV-2)? 1 : i+2; /*wrap-around, normally i+2 */
			count ++;
		}
		for(i=0;i<CONEDIV;i++)
		{
			/* bottom triangles */
			collisionCone.tris[count][0] = CONEDIV+1; /* bottom center point */
			collisionCone.tris[count][1] = i +1;
			collisionCone.tris[count][2] = i > (CONEDIV-2)?  1 : i+2; 
			count ++;
		}


	/* count should == num triangles 
	debug check on indexing - biggestNum should == npts -1 
	biggestNum = 0;
	for(i=0;i<collisionCone.ntris;i++)
		for(j=0;j<3;j++)
			biggestNum = max(biggestNum,collisionCone.tris[i][j]);
	*/
	/* MBB */
	for(i=0;i<3;i+=2)
	{
		collisionCone.smin[i] = -1.0; //r;
		collisionCone.smax[i] =  1.0; //r;
	}
	collisionCone.smin[1] = -1.0; //-h;
	collisionCone.smax[1] =  1.0; //h;
	
	/* ok - we copy the non-VBO code here so that Doug Sandens Cylinder Collision code
	   uses the same algorithm whether running in VBO mode or not */
	FREE_IF_NZ(botpoints.p);
    
}
#ifdef DEBUG_COLLISIONCONE
int collisionCone_render(double r, double h)
{
	/* I needed to verify the collision mesh was good, and it uses triangles, so I drew it the triangle way and it looked good 
	   to see it draw, you need to turn on collision and get close to the mesh object - then it will initialize and start drawing it.
	*/
	int i,j;
	for(i =0; i < collisionCone.ntris; i++)  
	{ 
		struct point_XYZ pts[3]; //,a,b,n;
		pts[0] = collisionCone.pts[collisionCone.tris[i][0]];
		pts[1] = collisionCone.pts[collisionCone.tris[i][1]];
		pts[2] = collisionCone.pts[collisionCone.tris[i][2]];
		glBegin(GL_TRIANGLES);
		for(j=0;j<3;j++)
			FW_GL_VERTEX3D(pts[j].x*r,pts[j].y*h,pts[j].z*r);
		glEnd();
	}
	return 0;
}
#endif
void collide_Cone (struct X3D_Cone *node) {

	       /*easy access, naviinfo.step unused for sphere collisions */
			ttglobal tg = gglobal();
	       GLDOUBLE awidth = tg->Bindable.naviinfo.width; /*avatar width*/
	       GLDOUBLE atop = tg->Bindable.naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLDOUBLE abottom = -tg->Bindable.naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLDOUBLE astep = -tg->Bindable.naviinfo.height+tg->Bindable.naviinfo.step;


                float h = (node->height) /2;
                float r = (node->bottomRadius) ;

	       GLDOUBLE modelMatrix[16];
	       struct point_XYZ iv = {0,0,0};
	       struct point_XYZ jv = {0,0,0};
	       GLDOUBLE scale = 0.0; /* FIXME: won''t work for non-uniform scales. */
	       struct point_XYZ t_orig = {0,0,0};

	       struct point_XYZ delta;

                /* is this node initialized? if not, get outta here and do this later */

		
			if (node->__coneVBO == 0) return;

	       iv.y = h; jv.y = -h;

	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

			//matmultiply(modelMatrix,FallInfo.avatar2collision,modelMatrix); 
			if(FallInfo()->walking)
			{
				/* mesh method */
				int i;
				double disp;
				struct point_XYZ n;
				struct point_XYZ a,b, dispv, maxdispv = {0,0,0};
				double maxdisp = 0;
				struct point_XYZ radscale;

				if(!collisionCone.npts) collisionCone_init(node);
#ifdef DEBUG_COLLISIONCONE
collisionCone_render(5.0,5.0);
#endif
				radscale.x = radscale.z = node->bottomRadius;
				radscale.y = node->height;
				scale_to_matrix (modelMatrix, &radscale);
				matmultiply(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
				//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 
				if( !avatarCollisionVolumeIntersectMBB(modelMatrix, collisionCone.smin,collisionCone.smax)) return;


				for(i=0;i<collisionCone.npts;i++)
					transform(&collisionCone.tpts[i],&collisionCone.pts[i],modelMatrix);
				for(i = 0; i < collisionCone.ntris; i++) 
				{
					/*only clip faces "facing" origin */
					//if(vecdot(&n[ci],&middle) < 0.) {
					{
						struct point_XYZ pts[3];
						pts[0] = collisionCone.tpts[collisionCone.tris[i][0]];
						pts[1] = collisionCone.tpts[collisionCone.tris[i][1]];
						pts[2] = collisionCone.tpts[collisionCone.tris[i][2]];
						/* compute normal - could compute once in shapespace then transform */
						VECDIFF(pts[1],pts[0],a);
						VECDIFF(pts[2],pts[1],b); /* or [2] [0] direction not sensitive for some functions */
						veccross(&n,a,b); /* 6 multiplies */
						vecnormal(&n,&n); 
						dispv = get_poly_disp_2(pts,3,n);
					    disp = vecdot(&dispv,&dispv);
						if( (disp > FLOAT_TOLERANCE) && (disp > maxdisp) ){
							maxdisp = disp;
							maxdispv = dispv;
						}
					}
				}
				delta = maxdispv;
			}
			else
			{
			   /* values for rapid test */
				matmultiply(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
				//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 

			   t_orig.x = modelMatrix[12];
			   t_orig.y = modelMatrix[13];
			   t_orig.z = modelMatrix[14];
			   scale = pow(det3x3(modelMatrix),1./3.);

			   if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;

			   /* get transformed box edges and position */
			   transform(&iv,&iv,modelMatrix);
			   transform(&jv,&jv,modelMatrix);

			   delta = cone_disp(abottom,atop,astep,awidth,jv,iv,scale*r);
			}
	       vecscale(&delta,&delta,-1);

	       accumulate_disp(CollisionInfo(),delta);

		#ifdef RENDERVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_CON: (%f %f %f) (%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  delta.x, delta.y, delta.z
			  );
	       if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) bR=%f\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  scale*r
			  );
		#endif
}

static void collisionCylinder_init(struct X3D_Cylinder *node)
{
	/* for debug ctri ct; */
	/* for debug struct point_XYZ a,b,n; */
	int i, tcount, qcount;
	/* for debug - int j,k,biggestNum; */
	double h,r,inverseh,inverser;
	struct SFVec3f *pts;// = node->__botpoints;
	
	/* not initialized yet - wait for next pass */

    if (!node->__points.p) return;
	/*  re-using the compile_cylinder node->__points data which is organized into GL_TRAIANGLE_FAN (bottom and top) 
	    and GL_QUADS (side)

		my understanding: 
		bottom and top: there are CYLDIV triangles arranged in a fan around a center point, with CYLDIV perimeter points 
		side: there are CYLDIV side quads formed with the top and bottom perimeter points
		num triangles: CYLDIV x 2
		num quads: CYLDIV
		num points: CYLDIV * 2 + center bottom + centre top = CYLDIV*2 +2 
		__points:
			pt[0- CYLDIV*2-1 + 2] - perimeter points including wrap-around points ordered as follows:
			    +h,-h,+h,-h .. vertical pair order, with 2 extra for easy wrap-around indexing
			pt[CYLDIV*2+2] - top center point of cylinder
			pt[CYLDIV*2+3] - centre of bottom
	*/
	collisionCylinder.npts = CYLDIV*2+2+2;
	collisionCylinder.pts = malloc(collisionCylinder.npts * sizeof(struct point_XYZ));
	collisionCylinder.tpts = malloc(collisionCylinder.npts * sizeof(struct point_XYZ));

	collisionCylinder.ntris = CYLDIV *2;
	collisionCylinder.tris = malloc(collisionCylinder.ntris * sizeof(ctri));
	collisionCylinder.nquads = CYLDIV;
	collisionCylinder.quads = malloc(collisionCylinder.nquads * sizeof(cquad));

	tcount = 0;
	qcount = 0;
	h = node->height;
	r = node->radius;
	inverseh = inverser = 1.0;
	if(!APPROX(h,0.0)) inverseh = 1.0/h;
	if(!APPROX(r,0.0)) inverser = 1.0/r;
	{
		float a1, a2;
		/* ok - we copy the non-VBO code here so that Doug Sandens Cylinder Collision code
		   uses the same algorithm whether running in VBO mode or not */
		pts = MALLOC(struct SFVec3f *,sizeof(struct SFVec3f)*2*(CYLDIV+4));
	
		/*  now, create the vertices; this is a quad, so each face = 4 points*/
		for (i=0; i<CYLDIV; i++) {
			a1 = (float) (PI*2*i)/(float)CYLDIV;
			a2 = (float) (PI*2*(i+1))/(float)CYLDIV;
			pts[i*2+0].c[0] = (float) (r* sin(a1));
			pts[i*2+0].c[1] = (float) h;
			pts[i*2+0].c[2] = (float) (r* cos(a1));
			pts[i*2+1].c[0] = (float) (r* sin(a1));
			pts[i*2+1].c[1] = (float) -h;
			pts[i*2+1].c[2] = (float) (r* cos(a1));
		}
	
		/*  wrap the points around*/
		memcpy (&pts[CYLDIV*2].c[0],&pts[0].c[0],sizeof(struct SFVec3f)*2);
	
		/*  center points of top and bottom*/
		pts[CYLDIV*2+2].c[0] = 0.0f; pts[CYLDIV*2+2].c[1] = (float) h; pts[CYLDIV*2+2].c[2] = 0.0f;
		pts[CYLDIV*2+3].c[0] = 0.0f; pts[CYLDIV*2+3].c[1] = (float)-h; pts[CYLDIV*2+3].c[2] = 0.0f;
	}
	for(i=0;i<collisionCylinder.npts;i++)
	{
		/* points */
		collisionCylinder.pts[i].x = pts[i].c[0]*inverser;
		collisionCylinder.pts[i].y = pts[i].c[1]*inverseh;
		collisionCylinder.pts[i].z = pts[i].c[2]*inverser;
	}
	for(i=0;i<CYLDIV;i++)
	{
		/* side quads */
		collisionCylinder.quads[qcount][0] = i*2;   /* top point */
		collisionCylinder.quads[qcount][1] = i*2 +1;
		collisionCylinder.quads[qcount][2] = i*2 +3; /*wrap-around, normally i+2 */
		collisionCylinder.quads[qcount][3] = i*2 +2;
		qcount ++;
	}
	//pt[CYLDIV*2+2].c[0] = 0.0; pt[CYLDIV*2+2].c[1] = (float) h; pt[CYLDIV*2+2].c[2] = 0.0;
	//pt[CYLDIV*2+3].c[0] = 0.0; pt[CYLDIV*2+3].c[1] = (float)-h; pt[CYLDIV*2+3].c[2] = 0.0;

	for(i=0;i<CYLDIV;i++)
	{
		/* bottom triangles */
		collisionCylinder.tris[tcount][0] = CYLDIV*2+3; /* bottom center point */
		collisionCylinder.tris[tcount][1] = i*2 +1;
		collisionCylinder.tris[tcount][2] = (i+1)*2 +1; 
		tcount ++;
	}
	for(i=0;i<CYLDIV;i++)
	{
		/* top triangles */
		collisionCylinder.tris[tcount][0] = CYLDIV*2+2; /* top center point */
		collisionCylinder.tris[tcount][1] = i*2;
		collisionCylinder.tris[tcount][2] = (i+1)*2; 
		tcount ++;
	}


	/* count should == num triangles 
	debug check on indexing - biggestNum should == npts -1 
	biggestNum = 0;
	for(i=0;i<collisionCylinder.ntris;i++)
		for(j=0;j<3;j++)
			biggestNum = max(biggestNum,collisionCylinder.tris[i][j]);
	*/
	/* MBB */
	for(i=0;i<3;i+=2)
	{
		collisionCylinder.smin[i] = -1.0; //r;
		collisionCylinder.smax[i] =  1.0; //r;
	}
	collisionCylinder.smin[1] = -1.0; //-h/2;
	collisionCylinder.smax[1] =  1.0; //h/2;


		FREE_IF_NZ(pts);
	
}

#ifdef DEBUGGING_CODE
DEBUGGING_CODEint collisionCylinder_render(double r, double h)
DEBUGGING_CODE{
DEBUGGING_CODE	/* I needed to verify the collision mesh was good, and it uses triangles, so I drew it the triangle way and it looked good 
DEBUGGING_CODE	   to see it draw, you need to turn on collision and get close to the mesh object - then it will initialize and start drawing it.
DEBUGGING_CODE	*/
DEBUGGING_CODE	int i,j;
DEBUGGING_CODE	for(i =0; i < collisionCylinder.ntris; i++)  
DEBUGGING_CODE	{ 
DEBUGGING_CODE		struct point_XYZ pts[3]; //,a,b,n;
DEBUGGING_CODE		pts[0] = collisionCylinder.pts[collisionCylinder.tris[i][0]];
DEBUGGING_CODE		pts[1] = collisionCylinder.pts[collisionCylinder.tris[i][1]];
DEBUGGING_CODE		pts[2] = collisionCylinder.pts[collisionCylinder.tris[i][2]];
DEBUGGING_CODE		FW_GL_BEGIN(GL_TRIANGLES);
DEBUGGING_CODE		for(j=0;j<3;j++)
DEBUGGING_CODE			FW_GL_VERTEX3D(pts[j].x*r,pts[j].y*h,pts[j].z*r);
DEBUGGING_CODE		FW_GL_END();
DEBUGGING_CODE	}
DEBUGGING_CODE	for(i =0; i < collisionCylinder.nquads; i++)  
DEBUGGING_CODE	{ 
DEBUGGING_CODE		struct point_XYZ pts[4]; //,a,b,n;
DEBUGGING_CODE		pts[0] = collisionCylinder.pts[collisionCylinder.quads[i][0]];
DEBUGGING_CODE		pts[1] = collisionCylinder.pts[collisionCylinder.quads[i][1]];
DEBUGGING_CODE		pts[2] = collisionCylinder.pts[collisionCylinder.quads[i][2]];
DEBUGGING_CODE		pts[3] = collisionCylinder.pts[collisionCylinder.quads[i][3]];
DEBUGGING_CODE		FW_GL_BEGIN(GL_QUADS);
DEBUGGING_CODE		for(j=0;j<4;j++)
DEBUGGING_CODE			FW_GL_VERTEX3D(pts[j].x*r,pts[j].y*h,pts[j].z*r);
DEBUGGING_CODE		FW_GL_END();
DEBUGGING_CODE	}
DEBUGGING_CODE	return 0;
DEBUGGING_CODE}
#endif


void collide_Cylinder (struct X3D_Cylinder *node) {
	       /*easy access, naviinfo.step unused for sphere collisions */
			ttglobal tg = gglobal();
	       GLDOUBLE awidth = tg->Bindable.naviinfo.width; /*avatar width*/
	       GLDOUBLE atop = tg->Bindable.naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLDOUBLE abottom = -tg->Bindable.naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLDOUBLE astep = -tg->Bindable.naviinfo.height+tg->Bindable.naviinfo.step;

                float h = (node->height)/2;
                float r = (node->radius);


	       GLDOUBLE modelMatrix[16];
	       struct point_XYZ iv = {0,0,0};
	       struct point_XYZ jv = {0,0,0};
	       GLDOUBLE scale=0.0; /* FIXME: won''t work for non-uniform scales. */
	       struct point_XYZ t_orig = {0,0,0};

	       struct point_XYZ delta;

		iv.y = h;
		jv.y = -h;

	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

			//matmultiply(modelMatrix,FallInfo.avatar2collision,modelMatrix); 
			if(FallInfo()->walking)
			{
				/* mesh method */
				int i;
				double disp;
				struct point_XYZ n;
				struct point_XYZ a,b, dispv, radscale, maxdispv = {0,0,0};
				double maxdisp = 0;

				if(!collisionCylinder.npts) collisionCylinder_init(node);
				radscale.x = radscale.z = node->radius;
				radscale.y = node->height;
				scale_to_matrix (modelMatrix, &radscale);
				matmultiply(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
				//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 
				if( !avatarCollisionVolumeIntersectMBB(modelMatrix, collisionCylinder.smin,collisionCylinder.smax)) return;

				for(i=0;i<collisionCylinder.npts;i++)
					transform(&collisionCylinder.tpts[i],&collisionCylinder.pts[i],modelMatrix);

				for(i = 0; i < collisionCylinder.ntris; i++) 
				{
					struct point_XYZ pts[3];
					pts[0] = collisionCylinder.tpts[collisionCylinder.tris[i][0]];
					pts[1] = collisionCylinder.tpts[collisionCylinder.tris[i][1]];
					pts[2] = collisionCylinder.tpts[collisionCylinder.tris[i][2]];
					/* compute normal - could compute once in shapespace then transform */
					VECDIFF(pts[1],pts[0],a);
					VECDIFF(pts[2],pts[1],b); /* or [2] [0] direction not sensitive for some functions */
					veccross(&n,a,b); /* 6 multiplies */
					vecnormal(&n,&n); 
					dispv = get_poly_disp_2(pts,3,n);
				    disp = vecdot(&dispv,&dispv);
					if( (disp > FLOAT_TOLERANCE) && (disp > maxdisp) ){
						maxdisp = disp;
						maxdispv = dispv;
					}
				}
				for(i = 0; i < collisionCylinder.nquads; i++) 
				{
					struct point_XYZ pts[4];
					pts[0] = collisionCylinder.tpts[collisionCylinder.quads[i][0]];
					pts[1] = collisionCylinder.tpts[collisionCylinder.quads[i][1]];
					pts[2] = collisionCylinder.tpts[collisionCylinder.quads[i][2]];
					pts[3] = collisionCylinder.tpts[collisionCylinder.quads[i][3]];
					/* compute normal - could compute once in shapespace then transform */
					VECDIFF(pts[1],pts[0],a);
					VECDIFF(pts[2],pts[1],b); /* or [2] [0] direction not sensitive for some functions */
					veccross(&n,a,b); /* 6 multiplies */
					vecnormal(&n,&n); 
					dispv = get_poly_disp_2(pts,4,n);
				    disp = vecdot(&dispv,&dispv);
					if( (disp > FLOAT_TOLERANCE) && (disp > maxdisp) ){
						maxdisp = disp;
						maxdispv = dispv;
					}
				}
				delta = maxdispv;
			}
			else
			{

			   /* values for rapid test */
				matmultiply(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
				//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 
			   t_orig.x = modelMatrix[12];
			   t_orig.y = modelMatrix[13];
			   t_orig.z = modelMatrix[14];
			   scale = pow(det3x3(modelMatrix),1./3.);
			   if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;



			   /* get transformed box edges and position */
			   transform(&iv,&iv,modelMatrix);
			   transform(&jv,&jv,modelMatrix);


			   delta = cylinder_disp(abottom,atop,astep,awidth,jv,iv,scale*r);
			}
	       vecscale(&delta,&delta,-1);

	       accumulate_disp(CollisionInfo(),delta);

		#ifdef RENDERVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_CYL: (%f %f %f) (%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  delta.x, delta.y, delta.z
			  );
	       if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) bR=%f\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  scale*r
			  );
		#endif
}

void collide_Extrusion (struct X3D_Extrusion *node) {
	       GLDOUBLE modelMatrix[16];
	       struct point_XYZ delta = {0,0,0};

		#ifdef RENDERVERBOSE
		struct point_XYZ t_orig = {0,0,0};
		#endif


	       struct X3D_PolyRep pr;
	       prflags flags = 0;
	       int change = 0;

		/* JAS - first pass, intern is probably zero */
		if (((struct X3D_PolyRep *)node->_intern) == 0) return;

		/* JAS - no triangles in this text structure */
		if ((((struct X3D_PolyRep *)node->_intern)->ntri) == 0) return;

	       /*save changed state.*/
	       if(node->_intern) change = ((struct X3D_PolyRep *)node->_intern)->irep_change;
                COMPILE_POLY_IF_REQUIRED(NULL, NULL, NULL, NULL)
 	       if(node->_intern) ((struct X3D_PolyRep *)node->_intern)->irep_change = change;
	       /*restore changes state, invalidates compile_polyrep work done, so it can be done
	         correclty in the RENDER pass */

	       if(!node->solid) {
		   flags = flags | PR_DOUBLESIDED;
	       }
/*	       printf("_PolyRep = %d\n",node->_intern);*/
	       pr = *((struct X3D_PolyRep*)node->_intern);
	       FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

   			matmultiply(modelMatrix,modelMatrix,FallInfo()->avatar2collision); 
			//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix); 

			#ifdef RENDERVERBOSE
                           t_orig.x = modelMatrix[12];
                           t_orig.y = modelMatrix[13];
                           t_orig.z = modelMatrix[14];
			#endif

			if(!avatarCollisionVolumeIntersectMBBf(modelMatrix, pr.minVals, pr.maxVals))return;

	       delta = polyrep_disp2(pr,modelMatrix,flags); 

	       vecscale(&delta,&delta,-1);

	       accumulate_disp(CollisionInfo(),delta);

		#ifdef RENDERVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
/*		   printmatrix(modelMatrix);*/
		   fprintf(stderr,"COLLISION_EXT: (%f %f %f) (%f %f %f)\n",
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z
			  );
	       }
		#endif
}



void rendray_Sphere (struct X3D_Sphere *node) {
	struct point_XYZ t_r1,t_r2,t_r3;
        float r = node->radius;
        /* Center is at zero. t_r1 to t_r2 and t_r1 to zero are the vecs */
		ttglobal tg = gglobal();
        float tr1sq;// = (float) VECSQ(t_r1);
        struct point_XYZ dr2r1;
        float dlen;
        float a,b,c,disc;
		VECCOPY(t_r1,tg->RenderFuncs.t_r1);
		VECCOPY(t_r2,tg->RenderFuncs.t_r2);
		VECCOPY(t_r3,tg->RenderFuncs.t_r3);
        tr1sq = (float) VECSQ(t_r1);

        VECDIFF(t_r2,t_r1,dr2r1);
        dlen = (float) VECSQ(dr2r1);

        a = dlen; /* tr1sq - 2*tr1tr2 + tr2sq; */
        b = 2.0f*((float)VECPT(dr2r1, t_r1));
        c = tr1sq - r*r;

        disc = b*b - 4*a*c; /* The discriminant */

        if(disc > 0) { /* HITS */
                float q ;
                float sol1 ;
                float sol2 ;
                float cx,cy,cz;
                q = (float) sqrt(disc);
                /* q = (-b+(b>0)?q:-q)/2; */
                sol1 = (-b+q)/(2*a);
                sol2 = (-b-q)/(2*a);
                /*
                printf("SPHSOL0: (%f %f %f) (%f %f %f)\n",
                        t_r1.x, t_r1.y, t_r1.z, t_r2.x, t_r2.y, t_r2.z);
                printf("SPHSOL: (%f %f %f) (%f) (%f %f) (%f) (%f %f)\n",
                        tr1sq, tr2sq, tr1tr2, a, b, c, und, sol1, sol2);
                */ 
                cx = (float) MRATX(sol1);
                cy = (float) MRATY(sol1);
                cz = (float) MRATZ(sol1);
                rayhit(sol1, cx,cy,cz, cx/r,cy/r,cz/r, -1,-1, "sphere 0");
                cx = (float) MRATX(sol2);
                cy = (float) MRATY(sol2);
                cz = (float) MRATZ(sol2);
                rayhit(sol2, cx,cy,cz, cx/r,cy/r,cz/r, -1,-1, "sphere 1");
        }

}


void rendray_Box (struct X3D_Box *node) {
	float x,y,z;
	struct point_XYZ t_r1,t_r2,t_r3;
	ttglobal tg = gglobal();
	VECCOPY(t_r1,tg->RenderFuncs.t_r1);
	VECCOPY(t_r2,tg->RenderFuncs.t_r2);
	VECCOPY(t_r3,tg->RenderFuncs.t_r3);

	x = ((node->size).c[0])/2;
	y = ((node->size).c[1])/2;
	z = ((node->size).c[2])/2;
	/* 1. x=const-plane faces? */
	if(!XEQ) {
		float xrat0 = (float) XRAT(x);
		float xrat1 = (float) XRAT(-x);
		#ifdef RENDERVERBOSE 
		printf("!XEQ: %f %f\n",xrat0,xrat1);
		#endif

		if(TRAT(xrat0)) {
			float cy = (float) MRATY(xrat0);
			#ifdef RENDERVERBOSE 
			printf("TRok: %f\n",cy);
			#endif

			if(cy >= -y && cy < y) {
				float cz = (float) MRATZ(xrat0);
				#ifdef RENDERVERBOSE 
				printf("cyok: %f\n",cz);
				#endif

				if(cz >= -z && cz < z) {
					#ifdef RENDERVERBOSE 
					printf("czok:\n");
					#endif

					rayhit(xrat0, x,cy,cz, 1,0,0, -1,-1, "cube x0");
				}
			}
		}
		if(TRAT(xrat1)) {
			float cy = (float) MRATY(xrat1);
			if(cy >= -y && cy < y) {
				float cz = (float) MRATZ(xrat1);
				if(cz >= -z && cz < z) {
					rayhit(xrat1, -x,cy,cz, -1,0,0, -1,-1, "cube x1");
				}
			}
		}
	}
	if(!YEQ) {
		float yrat0 = (float) YRAT(y);
		float yrat1 = (float) YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = (float) MRATX(yrat0);
			if(cx >= -x && cx < x) {
				float cz = (float) MRATZ(yrat0);
				if(cz >= -z && cz < z) {
					rayhit(yrat0, cx,y,cz, 0,1,0, -1,-1, "cube y0");
				}
			}
		}
		if(TRAT(yrat1)) {
			float cx = (float) MRATX(yrat1);
			if(cx >= -x && cx < x) {
				float cz = (float) MRATZ(yrat1);
				if(cz >= -z && cz < z) {
					rayhit(yrat1, cx,-y,cz, 0,-1,0, -1,-1, "cube y1");
				}
			}
		}
	}
	if(!ZEQ) {
		float zrat0 = (float) ZRAT(z);
		float zrat1 = (float) ZRAT(-z);
		if(TRAT(zrat0)) {
			float cx = (float) MRATX(zrat0);
			if(cx >= -x && cx < x) {
				float cy = (float) MRATY(zrat0);
				if(cy >= -y && cy < y) {
					rayhit(zrat0, cx,cy,z, 0,0,1, -1,-1,"cube z0");
				}
			}
		}
		if(TRAT(zrat1)) {
			float cx = (float) MRATX(zrat1);
			if(cx >= -x && cx < x) {
				float cy = (float) MRATY(zrat1);
				if(cy >= -y && cy < y) {
					rayhit(zrat1, cx,cy,-z, 0,0,-1,  -1,-1,"cube z1");
				}
			}
		}
	}
}


void rendray_Cylinder (struct X3D_Cylinder *node) {

	float h,r,y;
	struct point_XYZ t_r1,t_r2,t_r3;
	ttglobal tg = gglobal();
	VECCOPY(t_r1,tg->RenderFuncs.t_r1);
	VECCOPY(t_r2,tg->RenderFuncs.t_r2);
	VECCOPY(t_r3,tg->RenderFuncs.t_r3);

        h = (node->height) /*cget*//2; /* pos and neg dir. */
        r = (node->radius) /*cget*/;
        y = h;
        /* Caps */
        if(!YEQ) {
                float yrat0 = (float) YRAT(y);
                float yrat1 = (float) YRAT(-y);
                if(TRAT(yrat0)) {
                        float cx = (float) MRATX(yrat0);
                        float cz = (float) MRATZ(yrat0);
                        if(r*r > cx*cx+cz*cz) {
                                rayhit(yrat0, cx,y,cz, 0,1,0, -1,-1, "cylcap 0");
                        }
                }
                if(TRAT(yrat1)) {
                        float cx = (float) MRATX(yrat1);
                        float cz = (float) MRATZ(yrat1);
                        if(r*r > cx*cx+cz*cz) {
                                rayhit(yrat1, cx,-y,cz, 0,-1,0, -1,-1, "cylcap 1");
                        }
                }
        }
        /* Body -- do same as for sphere, except no y axis in distance */
        if((!XEQ) && (!ZEQ)) {
                float dx = (float)(t_r2.x-t_r1.x); 
		float dz = (float)(t_r2.z-t_r1.z);
                float a = (float)(dx*dx + dz*dz);
                float b = (float) (2*(dx * t_r1.x + dz * t_r1.z));
                float c = (float) (t_r1.x * t_r1.x + t_r1.z * t_r1.z - r*r);
                float und;
                b /= a; c /= a;
                und = b*b - 4*c;
                if(und > 0) { /* HITS the infinite cylinder */
                        float sol1 = (-b+(float) sqrt(und))/2;
                        float sol2 = (-b-(float) sqrt(und))/2;
                        float cy,cx,cz;
                        cy = (float) MRATY(sol1);
                        if(cy > -h && cy < h) {
                                cx = (float) MRATX(sol1);
                                cz = (float) MRATZ(sol1);
                                rayhit(sol1, cx,cy,cz, cx/r,0,cz/r, -1,-1, "cylside 1");
                        }
                        cy = (float) MRATY(sol2);
                        if(cy > -h && cy < h) {
                                cx = (float) MRATX(sol2);
                                cz = (float) MRATZ(sol2);
                                rayhit(sol2, cx,cy,cz, cx/r,0,cz/r, -1,-1, "cylside 2");
                        }
                }
        }
}

void rendray_Cone (struct X3D_Cone *node) {

	float h,y,r,dx,dy,dz,a,b,c,tmp,und;
	struct point_XYZ t_r1,t_r2,t_r3;
	ttglobal tg = gglobal();
	VECCOPY(t_r1,tg->RenderFuncs.t_r1);
	VECCOPY(t_r2,tg->RenderFuncs.t_r2);
	VECCOPY(t_r3,tg->RenderFuncs.t_r3);

	h = (node->height) /*cget*//2; /* pos and neg dir. */
	y = h;
	r = (node->bottomRadius) /*cget*/;
	dx = (float) (t_r2.x-t_r1.x); 
	dz = (float) (t_r2.z-t_r1.z);
	dy = (float) (t_r2.y-t_r1.y);
	a = dx*dx + dz*dz - (r*r*dy*dy/(2*h*2*h));
	b = (float) (2*(dx*t_r1.x + dz*t_r1.z) +
		2*r*r*dy/(2*h)*(0.5-t_r1.y/(2*h)));
	tmp = (float)((0.5-t_r1.y/(2*h)));
	c = (float)(t_r1.x * t_r1.x + t_r1.z * t_r1.z)
		- r*r*tmp*tmp;
	
	b /= a; c /= a;
	und = b*b - 4*c;
	/*
	printf("CONSOL0: (%f %f %f) (%f %f %f)\n",
		t_r1.x, t_r1.y, t_r1.z, t_r2.x, t_r2.y, t_r2.z);
	printf("CONSOL: (%f %f %f) (%f) (%f %f) (%f)\n",
		dx, dy, dz, a, b, c, und);
	*/
	if(und > 0) { /* HITS the infinite cylinder */
		float sol1 = (-b+(float)sqrt(und))/2;
		float sol2 = (-b-(float)sqrt(und))/2;
		float cy,cx,cz;
		float cy0;
		cy = (float)MRATY(sol1);
		if(cy > -h && cy < h) {
			cx = (float)MRATX(sol1);
			cz = (float)MRATZ(sol1);
			/* XXX Normal */
			rayhit(sol1, cx,cy,cz, cx/r,0,cz/r, -1,-1, "conside 1");
		}
		cy0 = cy;
		cy = (float) MRATY(sol2);
		if(cy > -h && cy < h) {
			cx = (float) MRATX(sol2);
			cz = (float) MRATZ(sol2);
			rayhit(sol2, cx,cy,cz, cx/r,0,cz/r, -1,-1, "conside 2");
		}
		/*
		printf("CONSOLV: (%f %f) (%f %f)\n", sol1, sol2,cy0,cy);
		*/
	}
	if(!YEQ) {
		float yrat0 = (float) YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = (float) MRATX(yrat0);
			float cz = (float) MRATZ(yrat0);
			if(r*r > cx*cx + cz*cz) {
				rayhit(yrat0, cx, -y, cz, 0, -1, 0, -1, -1, "conbot");
			}
		}
	}
}
