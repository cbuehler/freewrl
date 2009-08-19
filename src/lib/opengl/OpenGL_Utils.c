/*
=INSERT_TEMPLATE_HERE=

$Id: OpenGL_Utils.c,v 1.53 2009/08/19 04:10:33 dug9 Exp $

???

*/

#include <config.h>
#include <system.h>
#include <system_threads.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../main/ProdCon.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/CScripts.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CParse.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../scenegraph/sounds.h"
#include "../scenegraph/LinearAlgebra.h"
#include "../input/InputFunctions.h"
#include "../input/EAIheaders.h"
#include "Frustum.h"
#include "../opengl/Material.h"

#include <float.h>

#include "../x3d_parser/Bindable.h"


void kill_rendering(void);

/* Node Tracking */
void kill_X3DNodes(void);
static void createdMemoryTable();
static void increaseMemoryTable();
static struct X3D_Node ** memoryTable = NULL;
static int tableIndexSize = INT_ID_UNDEFINED;
static int nextEntry = 0;
static struct X3D_Node *forgottenNode;

/* lights status. Light 0 is the headlight */
static int lights[8];

/* is this 24 bit depth? 16? 8?? Assume 24, unless set on opening */
int displayDepth = 24;

int opengl_has_shaders = FALSE;
int opengl_has_multitexture = FALSE;
int opengl_has_occlusionQuery = FALSE;
int opengl_has_numTextureUnits = 0;
GLint opengl_has_textureSize = 0;

static float cc_red = 0.0f, cc_green = 0.0f, cc_blue = 0.0f, cc_alpha = 1.0f;
int cc_changed = FALSE;

/* for checking of runtime capabilities */
static char *glExtensions;

static pthread_mutex_t  memtablelock = PTHREAD_MUTEX_INITIALIZER;
/*
#define LOCK_MEMORYTABLE
#define UNLOCK_MEMORYTABLE
*/
#define LOCK_MEMORYTABLE 		pthread_mutex_lock(&memtablelock);
#define UNLOCK_MEMORYTABLE		pthread_mutex_unlock(&memtablelock);


/******************************************************************/
/* textureTransforms of all kinds */

/* change the clear colour, selected from the GUI, but do the command in the
   OpenGL thread */

void setglClearColor (float *val) {
	cc_red = *val; val++;
	cc_green = *val; val++;
	cc_blue = *val;
#ifdef AQUA
	val++;
	cc_alpha = *val;
#endif
	cc_changed = TRUE;
}        


/**************************************************************************************

		Determine near far clip planes.

We have 2 choices; normal geometry, or we have a Geospatial sphere.

If we have normal geometry (normal Viewpoint, or GeoViewpoint with GC coordinates)
then, we take our AABB (axis alligned bounding box), rotate the 8 Vertices, and
find the min/max Z distance, and just use this. 

This works very well for examine objects, or when we are within a virtual world.
----

If we are Geospatializing around the earth, so we have GeoSpatial and have UTM or GD
coordinates, lets do some optimizations here.

First optimization, we know our height above the origin, and we most certainly are not
going to see things past the origin, so we assume far plane is height above the origin.

Second, we know our AABB contains the Geospatial sphere, and it ALSO contains the highest
mountain peak, so we just go and find the value representing the highest peak. Our
near plane is thus farPlane - highestPeak. 

**************************************************************************************/

static void handle_GeoLODRange(struct X3D_GeoLOD *node) {
	int oldInRange;
	double cx,cy,cz;

	/* find the length of the line between the moved center and our current viewer position */
	cx = Viewer.currentPosInModel.x - node->__movedCoords.c[0];
	cy = Viewer.currentPosInModel.y - node->__movedCoords.c[1];
	cz = Viewer.currentPosInModel.z - node->__movedCoords.c[2];

	/* printf ("geoLOD, distance between me and center is %lf\n", sqrt (cx*cx + cy*cy + cz*cz)); */

	/* try to see if we are closer than the range */
	oldInRange = node->__inRange;
	if((cx*cx+cy*cy+cz*cz) > (node->range*X3D_GEOLOD(node)->range)) {
		node->__inRange = FALSE;
	} else {
		node->__inRange = TRUE;
	}

	
	if (oldInRange != node->__inRange) {

		#ifdef VERBOSE
		if (node->__inRange) printf ("TRUE:  "); else printf ("FALSE: ");
		printf ("range changed; level %d, comparing %lf:%lf:%lf and range %lf node %u\n",
			node->__level, cx,cy,cz, node->range, node);
		#endif

		/* initialize the "children" field, if required */
		if (node->children.p == NULL) node->children.p=MALLOC(sizeof(void *) * 4);

		if (node->__inRange == FALSE) {
			#ifdef VERBOSE
			printf ("GeoLOD %u level %d, inRange set to FALSE, range %lf\n",node, node->__level, node->range);
			#endif

			node->level_changed = 1;
			node->children.p[0] = node->__child1Node; 
			node->children.p[1] = node->__child2Node; 
			node->children.p[2] = node->__child3Node; 
			node->children.p[3] = node->__child4Node; 
			node->children.n = 4;
		} else {
			#ifdef VERBOSE
			printf ("GeoLOD %u level %d, inRange set to TRUE range %lf\n",node, node->__level, node->range);
			#endif
			node->level_changed = 0;
			node->children.p[0] = node->rootNode.p[0]; node->children.n = 1;
		}
		MARK_EVENT(X3D_NODE(node), offsetof (struct X3D_GeoLOD, level_changed));
		MARK_EVENT(X3D_NODE(node), offsetof (struct X3D_GeoLOD, children));
		oldInRange = X3D_GEOLOD(node)->__inRange;

		/* lets work out extent here */
		INITIALIZE_EXTENT;
		/* printf ("geolod range changed, initialized extent, czyzsq %4.2f rangesw %4.2f from %4.2f %4.2f %4.2f\n",
cx*cx+cy*cy+cz*cz,node->range*node->range,cx,cy,cz); */
		update_node(X3D_NODE(node));
	}
}





/* draw a simple bounding box around an object */
void drawBBOX(struct X3D_Node *node) {
	glDisable(GL_LIGHTING);
	glColor3f(1.0,0.6,0.6);

	/* left group */
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
	glEnd();
	
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
	glEnd();
	
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
	glEnd();
	
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
	glEnd();
	
	/* right group */
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
	glEnd();
	
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
	glEnd();
	
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
	glEnd();
	
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
	glEnd();
	
	/* joiners */
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
	glEnd();
	
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
	glEnd();
	
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
	glEnd();
	
	glBegin (GL_LINES);
	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
	glEnd();

	glEnable (GL_LIGHTING);
}


static void calculateNearFarplanes(struct X3D_Node *vpnode) {
	struct point_XYZ bboxPoints[8];
	double cfp = DBL_MIN;
	double cnp = DBL_MAX;
	GLdouble MM[16];

	int ci;

	#ifdef VERBOSE
	printf ("have a bound viewpoint... lets calculate our near/far planes from it \n");
	printf ("we are currently at %4.2f %4.2f %4.2f\n",Viewer.currentPosInModel.x, Viewer.currentPosInModel.y, Viewer.currentPosInModel.z);
	#endif


	/* verify parameters here */
	if ((vpnode->_nodeType != NODE_Viewpoint) && (vpnode->_nodeType != NODE_GeoViewpoint)) {
		printf ("can not do this node type yet %s, for cpf\n",stringNodeType(vpnode->_nodeType));
		nearPlane = DEFAULT_NEARPLANE;
		farPlane = DEFAULT_FARPLANE;
		backgroundPlane = DEFAULT_BACKGROUNDPLANE;
		return;
	}	

	if (rootNode == NULL) {
		return; /* nothing to display yet */
	}

	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, MM);

		#ifdef VERBOSE
		printf ("rootNode extents x: %4.2f %4.2f  y:%4.2f %4.2f z: %4.2f %4.2f\n",
				X3D_GROUP(rootNode)->EXTENT_MAX_X, X3D_GROUP(rootNode)->EXTENT_MIN_X,
				X3D_GROUP(rootNode)->EXTENT_MAX_Y, X3D_GROUP(rootNode)->EXTENT_MIN_Y,
				X3D_GROUP(rootNode)->EXTENT_MAX_Z, X3D_GROUP(rootNode)->EXTENT_MIN_Z);
		#endif
		/* make up 8 vertices for our bounding box, and place them within our view */
		moveAndRotateThisPoint(&bboxPoints[0], X3D_GROUP(rootNode)->EXTENT_MIN_X, X3D_GROUP(rootNode)->EXTENT_MIN_Y, X3D_GROUP(rootNode)->EXTENT_MIN_Z,MM);
		moveAndRotateThisPoint(&bboxPoints[1], X3D_GROUP(rootNode)->EXTENT_MIN_X, X3D_GROUP(rootNode)->EXTENT_MIN_Y, X3D_GROUP(rootNode)->EXTENT_MAX_Z,MM);
		moveAndRotateThisPoint(&bboxPoints[2], X3D_GROUP(rootNode)->EXTENT_MIN_X, X3D_GROUP(rootNode)->EXTENT_MAX_Y, X3D_GROUP(rootNode)->EXTENT_MIN_Z,MM);
		moveAndRotateThisPoint(&bboxPoints[3], X3D_GROUP(rootNode)->EXTENT_MIN_X, X3D_GROUP(rootNode)->EXTENT_MAX_Y, X3D_GROUP(rootNode)->EXTENT_MAX_Z,MM);
		moveAndRotateThisPoint(&bboxPoints[4], X3D_GROUP(rootNode)->EXTENT_MAX_X, X3D_GROUP(rootNode)->EXTENT_MIN_Y, X3D_GROUP(rootNode)->EXTENT_MIN_Z,MM);
		moveAndRotateThisPoint(&bboxPoints[5], X3D_GROUP(rootNode)->EXTENT_MAX_X, X3D_GROUP(rootNode)->EXTENT_MIN_Y, X3D_GROUP(rootNode)->EXTENT_MAX_Z,MM);
		moveAndRotateThisPoint(&bboxPoints[6], X3D_GROUP(rootNode)->EXTENT_MAX_X, X3D_GROUP(rootNode)->EXTENT_MAX_Y, X3D_GROUP(rootNode)->EXTENT_MIN_Z,MM);
		moveAndRotateThisPoint(&bboxPoints[7], X3D_GROUP(rootNode)->EXTENT_MAX_X, X3D_GROUP(rootNode)->EXTENT_MAX_Y, X3D_GROUP(rootNode)->EXTENT_MAX_Z,MM);
	
		for (ci=0; ci<8; ci++) {
			#ifdef VERBOSE
			printf ("moved bbox node %d is %4.2f %4.2f %4.2f\n",ci,bboxPoints[ci].x, bboxPoints[ci].y, bboxPoints[ci].z);
			#endif
	
			if (-(bboxPoints[ci].z) > cfp) cfp = -(bboxPoints[ci].z);
			if (-(bboxPoints[ci].z) < cnp) cnp = -(bboxPoints[ci].z);
		}

	/* lets bound check here, both must be positive, and farPlane more than DEFAULT_NEARPLANE */
	/* because we may be navigating towards the shapes, we give the nearPlane a bit of room, otherwise
	   we might miss part of the geometry that comes closest to us */
	cnp = cnp/2.0;
	if (cnp<DEFAULT_NEARPLANE) cnp = DEFAULT_NEARPLANE;

	/* we could (probably should) keep the farPlane at 1.0 and above, but because of an Examine mode 
	   rotation problem, we will just keep it at 2100 for now. */
	if (cfp<1.0) cfp = 1.0;	

	#ifdef VERBOSE
	printf ("cnp %lf cfp before leaving room for Background %lf\n",cnp,cfp);
	#endif

	/* lets use these values; leave room for a Background or TextureBackground node here */
	nearPlane = cnp; 
	/* backgroundPlane goes between the farthest geometry, and the farPlane */
	if (background_stack[0]!= 0) {
		farPlane = cfp * 10.0;
		backgroundPlane = cfp*5.0;
	} else {
		farPlane = cfp;
		backgroundPlane = cfp; /* just set it to something */
	}
}

void doglClearColor() {
	glClearColor(cc_red, cc_green, cc_blue, cc_alpha);
	cc_changed = FALSE;
}


/* if we had an opengl error... */
void glPrintError(char *str) {
        if (displayOpenGLErrors) {
                int err;
                while((err = glGetError()) != GL_NO_ERROR)
                        printf("OpenGL Error: \"%s\" in %s\n", gluErrorString((unsigned)err),str);
        }
}


/* did we have a TextureTransform in the Appearance node? */
void start_textureTransform (struct X3D_Node *textureNode, int ttnum) {
	

	/* stuff common to all textureTransforms - gets undone at end_textureTransform */
	FW_GL_MATRIX_MODE(GL_TEXTURE);
       	/* done in RenderTextures now glEnable(GL_TEXTURE_2D); */
	FW_GL_LOAD_IDENTITY();

	/* is this a simple TextureTransform? */
	if (textureNode->_nodeType == NODE_TextureTransform) {
		struct X3D_TextureTransform  *ttt = (struct X3D_TextureTransform *) textureNode;
		/*  Render transformations according to spec.*/
        	FW_GL_TRANSLATE_F(-((ttt->center).c[0]),-((ttt->center).c[1]), 0);		/*  5*/
        	FW_GL_SCALE_F(((ttt->scale).c[0]),((ttt->scale).c[1]),1);			/*  4*/
        	FW_GL_ROTATE_F((ttt->rotation) /3.1415926536*180,0,0,1);			/*  3*/
        	FW_GL_TRANSLATE_F(((ttt->center).c[0]),((ttt->center).c[1]), 0);		/*  2*/
        	FW_GL_TRANSLATE_F(((ttt->translation).c[0]), ((ttt->translation).c[1]), 0);	/*  1*/

	/* is this a MultiTextureTransform? */
	} else  if (textureNode->_nodeType == NODE_MultiTextureTransform) {
		struct X3D_MultiTextureTransform *mtt = (struct X3D_MultiTextureTransform *) textureNode;
		if (ttnum < mtt->textureTransform.n) {
			struct X3D_TextureTransform *ttt = (struct X3D_TextureTransform *) mtt->textureTransform.p[ttnum];
			/* is this a simple TextureTransform? */
			if (ttt->_nodeType == NODE_TextureTransform) {
				/*  Render transformations according to spec.*/
        			FW_GL_TRANSLATE_F(-((ttt->center).c[0]),-((ttt->center).c[1]), 0);		/*  5*/
        			FW_GL_SCALE_F(((ttt->scale).c[0]),((ttt->scale).c[1]),1);			/*  4*/
        			FW_GL_ROTATE_F((ttt->rotation) /3.1415926536*180,0,0,1);			/*  3*/
        			FW_GL_TRANSLATE_F(((ttt->center).c[0]),((ttt->center).c[1]), 0);		/*  2*/
        			FW_GL_TRANSLATE_F(((ttt->translation).c[0]), ((ttt->translation).c[1]), 0);	/*  1*/
			} else {
				printf ("MultiTextureTransform expected a textureTransform for texture %d, got %d\n",
					ttnum, ttt->_nodeType);
			}
		} else {
			printf ("not enough textures in MultiTextureTransform....\n");
		}

	} else if (textureNode->_nodeType == NODE_VRML1_Texture2Transform) {
		struct X3D_VRML1_Texture2Transform  *ttt = (struct X3D_VRML1_Texture2Transform *) textureNode;
		/*  Render transformations according to spec.*/
        	FW_GL_TRANSLATE_F(-((ttt->center).c[0]),-((ttt->center).c[1]), 0);		/*  5*/
        	FW_GL_SCALE_F(((ttt->scaleFactor).c[0]),((ttt->scaleFactor).c[1]),1);			/*  4*/
        	FW_GL_ROTATE_F((ttt->rotation) /3.1415926536*180,0,0,1);			/*  3*/
        	FW_GL_TRANSLATE_F(((ttt->center).c[0]),((ttt->center).c[1]), 0);		/*  2*/
        	FW_GL_TRANSLATE_F(((ttt->translation).c[0]), ((ttt->translation).c[1]), 0);	/*  1*/
	} else {
		printf ("expected a textureTransform node, got %d\n",textureNode->_nodeType);
	}
	FW_GL_MATRIX_MODE(GL_MODELVIEW);
}

/* did we have a TextureTransform in the Appearance node? */
void end_textureTransform (void) {
	FW_GL_MATRIX_MODE(GL_TEXTURE);
	FW_GL_LOAD_IDENTITY();
	FW_GL_MATRIX_MODE(GL_MODELVIEW);
}

/* keep track of lighting */
void lightState(GLint light, int status) {
	if (light<0) return; /* nextlight will return -1 if too many lights */
	if (lights[light] != status) {
		if (status) glEnable(GL_LIGHT0+light);
		else glDisable(GL_LIGHT0+light);
		lights[light]=status;
	}
}

/* for local lights, we keep track of what is on and off */
void saveLightState(int *ls) {
	int i;
	for (i=0; i<7; i++) ls[i] = lights[i];
} 

void restoreLightState(int *ls) {
	int i;
	for (i=0; i<7; i++) {
		if (ls[i] != lights[i]) {
			lightState(i,ls[i]);
		}
	}
}


void glpOpenGLInitialize() {
	int i;
        float pos[] = { 0.0, 0.0, 1.0, 0.0 }; 
        float dif[] = { 1.0, 1.0, 1.0, 1.0 };
        float shin[] = { 0.6, 0.6, 0.6, 1.0 };
        float As[] = { 0.0, 0.0, 0.0, 1.0 };
	int checktexsize;

        #ifdef AQUA
	/* aqglobalContext is found at the initGL routine in MainLoop.c. Here
	   we make it the current Context. */

        /* printf("OpenGL at start of glpOpenGLInitialize globalContext %p\n", aqglobalContext); */
        if (RUNNINGASPLUGIN) {
                aglSetCurrentContext(aqglobalContext);
        } else {
                CGLSetCurrentContext(myglobalContext);
        }

        /* already set aqglobalContext = CGLGetCurrentContext(); */
        /* printf("OpenGL globalContext %p\n", aqglobalContext); */
        #endif

	/* Configure OpenGL for our uses. */
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
	glClearColor(cc_red, cc_green, cc_blue, cc_alpha);
	glShadeModel(GL_SMOOTH);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glLineWidth(gl_linewidth);
	glPointSize (gl_linewidth);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glEnable (GL_RESCALE_NORMAL);

	/*
     * JAS - ALPHA testing for textures - right now we just use 0/1 alpha
     * JAS   channel for textures - true alpha blending can come when we sort
     * JAS   nodes.
	 */

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_COLOR_BUFFER_BIT);

	/* end of ALPHA test */
	glEnable(GL_NORMALIZE);
	LIGHTING_INITIALIZE

	glEnable (GL_COLOR_MATERIAL);
	glColorMaterial (GL_FRONT_AND_BACK, GL_DIFFUSE);

	/* keep track of light states; initial turn all lights off except for headlight */
	for (i=0; i<8; i++) {
		lights[i] = 9999;
		lightState(i,FALSE);
	}
	/* headlight is GL_LIGHT7 */
	lightState(HEADLIGHT_LIGHT, TRUE);

        glLightfv(GL_LIGHT0+HEADLIGHT_LIGHT, GL_POSITION, pos);
        glLightfv(GL_LIGHT0+HEADLIGHT_LIGHT, GL_AMBIENT, As);
        glLightfv(GL_LIGHT0+HEADLIGHT_LIGHT, GL_DIFFUSE, dif);
        glLightfv(GL_LIGHT0+HEADLIGHT_LIGHT, GL_SPECULAR, shin);

	/* ensure state of GL_CULL_FACE */
	CULL_FACE_INITIALIZE

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,As);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_ALIGNMENT,1);

	do_shininess(GL_FRONT_AND_BACK,0.2);

	/* get extensions for runtime */
	printf ("OpenGL - getting extensions\n");
	
        glExtensions = (char *)glGetString(GL_EXTENSIONS);

	/* Shaders */
        opengl_has_shaders = ((strstr (glExtensions, "GL_ARB_fragment_shader")!=0)  &&
                (strstr (glExtensions,"GL_ARB_vertex_shader")!=0));

	/* Multitexturing */
        opengl_has_multitexture = (strstr (glExtensions, "GL_ARB_multitexture")!=0);

	/* Occlusion Queries */
	opengl_has_occlusionQuery = (strstr (glExtensions, "GL_ARB_occlusion_query") !=0);

	
	/* first, lets check to see if we have a max texture size yet */

	/* note - we reduce the max texture size on computers with the (incredibly inept) Intel GMA 9xx chipsets - like the Intel
	   Mac minis, and macbooks up to November 2007 */
	if (opengl_has_textureSize<=0) { 
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &checktexsize); 
		if (getenv("FREEWRL_256x256_TEXTURES")!= NULL) checktexsize = 256; 
		if (getenv("FREEWRL_512x512_TEXTURES")!= NULL) checktexsize = 512; 
		opengl_has_textureSize = -opengl_has_textureSize; 
		if (opengl_has_textureSize == 0) opengl_has_textureSize = checktexsize; 
		if (opengl_has_textureSize > checktexsize) opengl_has_textureSize = checktexsize; 
		if (strncmp((const char *)glGetString(GL_RENDERER),"NVIDIA GeForce2",strlen("NVIDIA GeForce2")) == 0) { 
		/* 	printf ("possibly reducing texture size because of NVIDIA GeForce2 chip\n"); */ 
			if (opengl_has_textureSize > 1024) opengl_has_textureSize = 1024; 
		}  
		if (strncmp((const char *)glGetString(GL_RENDERER),"Intel GMA 9",strlen("Intel GMA 9")) == 0) { 
		/* 	printf ("possibly reducing texture size because of Intel GMA chip\n"); */
			if (opengl_has_textureSize > 1024) opengl_has_textureSize = 1024;
		}
		if (displayOpenGLErrors) printf ("CHECK_MAX_TEXTURE_SIZE, ren %s ver %s ven %s ts %d\n",glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_VENDOR),opengl_has_textureSize);
		setMenuButton_texSize (opengl_has_textureSize);
	} 


	/* how many Texture units? */
	if ((strstr (glExtensions, "GL_ARB_texture_env_combine")!=0) &&
		(strstr (glExtensions,"GL_ARB_multitexture")!=0)) {

		glGetIntegerv(GL_MAX_TEXTURE_UNITS,&opengl_has_numTextureUnits);

		if (opengl_has_numTextureUnits > MAX_MULTITEXTURE) {
			printf ("init_multitexture_handling - reducing number of multitexs from %d to %d\n",
				opengl_has_numTextureUnits,MAX_MULTITEXTURE);
			opengl_has_numTextureUnits = MAX_MULTITEXTURE;
		}
		printf ("can do multitexture we have %d units\n",opengl_has_numTextureUnits);


		/* we assume that GL_TEXTURE*_ARB are sequential. Lets to a little check */
		if ((GL_TEXTURE0 +1) != GL_TEXTURE1) {
			printf ("Warning, code expects GL_TEXTURE0 to be 1 less than GL_TEXTURE1\n");
		} 
	}


#define VERBOSE
	#ifdef VERBOSE
	{
		char *p = glExtensions;
		while (*p != '\0') {
			if (*p == ' ') *p = '\n';
			p++;
		}
	}
	printf ("extensions %s\n",glExtensions);
	printf ("Shader support:       "); if (opengl_has_shaders) printf ("TRUE\n"); else printf ("FALSE\n");
	printf ("Multitexture support: "); if (opengl_has_multitexture) printf ("TRUE\n"); else printf ("FALSE\n");
	printf ("Occlusion support:    "); if (opengl_has_occlusionQuery) printf ("TRUE\n"); else printf ("FALSE\n");
	printf ("max texture size      %d\n",opengl_has_textureSize);
	printf ("texture units         %d\n",opengl_has_numTextureUnits);
	#endif	

#undef VERBOSE

}

void BackEndClearBuffer(int which) {
	if(which == 2)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else if(which==1)
		glClear(GL_DEPTH_BUFFER_BIT);
}

/* turn off all non-headlight lights; will turn them on if required. */
void BackEndLightsOff() {
	int i;
	for (i=0; i<7; i++) {
		lightState(i, FALSE);
	}
}

/* OpenGL tuning stuff - cache the modelview matrix */

static int myMat = -111;
static int MODmatOk = FALSE;
static int PROJmatOk = FALSE;
static double MODmat[16];
static double PROJmat[16];
#ifdef DEBUGCACHEMATRIX
static int sav = 0;
static int tot = 0;
#endif

void invalidateCurMat() {
	return;

	if (myMat == GL_PROJECTION) PROJmatOk=FALSE;
	else if (myMat == GL_MODELVIEW) MODmatOk=FALSE;
	else {
		printf ("fwLoad, unknown %d\n",myMat);
	}
}

void fwLoadIdentity () {
	FW_GL_LOAD_IDENTITY();
	invalidateCurMat();
}
	
void fwMatrixMode (int mode) {
	if (myMat != mode) {
		/* printf ("FW_GL_MATRIX_MODE ");
		if (mode == GL_PROJECTION) printf ("GL_PROJECTION\n");
		else if (mode == GL_MODELVIEW) printf ("GL_MODELVIEW\n");
		else {printf ("unknown %d\n",mode);}
		*/
		

		myMat = mode;
		FW_GL_MATRIX_MODE(mode);
	}
}

#ifdef DEBUGCACHEMATRIX
void pmat (double *mat) {
	int i;
	for (i=0; i<16; i++) {
		printf ("%3.2f ",mat[i]);
	}
	printf ("\n");
}

void compare (char *where, double *a, double *b) {
	int count;
	double va, vb;

	for (count = 0; count < 16; count++) {
		va = a[count];
		vb = b[count];
		if (fabs(va-vb) > 0.001) {
			printf ("%s difference at %d %lf %lf\n",
					where,count,va,vb);
		}

	}
}
#endif

void fwGetDoublev (int ty, double *mat) {

	/* we were trying to cache OpenGL gets, but, over time this code
	   has not worked too well as some rotates, etc, did not invalidate
	   our local flag. Just ignore the optimizations for now */
	glGetDoublev(ty,mat);
	return;

#ifdef DEBUGCACHEMATRIX
	double TMPmat[16];
	/*printf (" sav %d tot %d\n",sav,tot); */
	tot++;

#endif


	if (ty == GL_MODELVIEW_MATRIX) {
		if (!MODmatOk) {
			FW_GL_GETDOUBLEV (ty, MODmat);
			MODmatOk = TRUE;

#ifdef DEBUGCACHEMATRIX
		} else sav ++;

		// debug memory calls
		glGetDoublev(ty,TMPmat);
		compare ("MODELVIEW", TMPmat, MODmat);
		memcpy (MODmat,TMPmat, sizeof (MODmat));
		// end of debug
#else
		}
#endif

		memcpy (mat, MODmat, sizeof (MODmat));

	} else if (ty == GL_PROJECTION_MATRIX) {
		if (!PROJmatOk) {
			FW_GL_GETDOUBLEV (ty, PROJmat);
			PROJmatOk = TRUE;
#ifdef DEBUGCACHEMATRIX
		} else sav ++;

		// debug memory calls
		glGetDoublev(ty,TMPmat);
		compare ("PROJECTION", TMPmat, PROJmat);
		memcpy (PROJmat,TMPmat, sizeof (PROJmat));
		// end of debug
#else
		}
#endif

		memcpy (mat, PROJmat, sizeof (PROJmat));
	} else {
		printf ("fwGetDoublev, inv type %d\n",ty);
	}
}

#ifdef WIN32
/* FIXME: why use a C++ compiler ??? */
/* inline is cpp, ms has __inline for C */
__inline void fwXformPush(void) {
	FW_GL_PUSH_MATRIX(); 
	MODmatOk = FALSE;
}

__inline void fwXformPop(void) {
	FW_GL_POP_MATRIX(); 
	MODmatOk = FALSE;
}

#else
inline void fwXformPush(void) {
	FW_GL_PUSH_MATRIX(); 
	MODmatOk = FALSE;
}

inline void fwXformPop(void) {
	FW_GL_POP_MATRIX(); 
	MODmatOk = FALSE;
}
#endif
/* for Sarah's front end - should be removed sometime... */
void kill_rendering() {kill_X3DNodes();}


/* if we have a ReplaceWorld style command, we have to remove the old world. */
/* NOTE: There are 2 kinds of of replaceworld commands - sometimes we have a URL
   (eg, from an Anchor) and sometimes we have a list of nodes (from a Javascript
   replaceWorld, for instance). The URL ones can really replaceWorld, the node
   ones, really, it's just replace the rootNode children, as WE DO NOT KNOW
   what the user has programmed, and what nodes are (re) used in the Scene Graph */

void kill_oldWorld(int kill_EAI, int kill_JavaScript, int loadedFromURL, char *file, int line) {
	#ifndef AQUA
        char mystring[20];
	#endif

	/* close the Console Message system, if required. */
	closeConsoleMessage();

	/* occlusion testing - zero total count, but keep MALLOC'd memory around */
	zeroOcclusion();

	/* clock events - stop them from ticking */
	kill_clockEvents();

	/* kill DEFS, handles */
	EAI_killBindables();
	kill_bindables();
	killKeySensorNodeList();

	/* stop routing */
	kill_routing();

	/* stop rendering */
	X3D_GROUP(rootNode)->children.n = 0;

	/* tell the statusbar that it needs to reinitialize */
	kill_status();

	/* free textures */
	kill_openGLTextures();
	
	/* free scripts */
	kill_javascript();

	/* free EAI */
	if (kill_EAI) {
	       	shutdown_EAI();
	}

	/* free memory */
	kill_X3DNodes();

	/* kill the shadowFileTable, if it exists */
	kill_shadowFileTable();

	#ifndef AQUA
		sprintf (mystring, "QUIT");
		Sound_toserver(mystring);
	#endif

	/* reset any VRML Parser data */
	if (globalParser != NULL) {
		parser_destroyData(globalParser);
		globalParser = NULL;
	}

	/* tell statusbar that we have none */
	viewer_default();
	setMenuStatus("NONE");
}

/* for verifying that a memory pointer exists */
int checkNode(struct X3D_Node *node, char *fn, int line) {
	int tc;

	if (node == NULL) {
		printf ("checkNode, node is NULL at %s %d\n",fn,line);
		return FALSE;
	}

	if (node == forgottenNode) return TRUE;


	LOCK_MEMORYTABLE
	for (tc = 0; tc< nextEntry; tc++)
		if (memoryTable[tc] == node) {
			UNLOCK_MEMORYTABLE
			return TRUE;
	}


	printf ("checkNode: did not find %d in memory table at i%s %d\n",(int) node,fn,line);

	UNLOCK_MEMORYTABLE
	return FALSE;
}


/*keep track of node created*/
void registerX3DNode(struct X3D_Node * tmp){	
	LOCK_MEMORYTABLE
	/* printf("nextEntry=%d	",nextEntry); printf("tableIndexSize=%d \n",tableIndexSize); */
	/*is table to small give us some leeway in threads */
	if (nextEntry >= (tableIndexSize-10)){
		/*is table exist*/	
		if (tableIndexSize <= INT_ID_UNDEFINED){
			createdMemoryTable();		
		} else {
			increaseMemoryTable();
		}
	}
	/*adding node in table*/	
	/* printf ("registerX3DNode, adding %x at %d\n",tmp,nextEntry); */
	memoryTable[nextEntry] = tmp;
	nextEntry+=1;
	UNLOCK_MEMORYTABLE
}

/*We don't register the first node created for reload reason*/
void doNotRegisterThisNodeForDestroy(struct X3D_Node * nodePtr){
	LOCK_MEMORYTABLE
	if(nodePtr==(memoryTable[nextEntry-1])){
		nextEntry-=1;
		forgottenNode = nodePtr;
	}	
	UNLOCK_MEMORYTABLE
}

/*creating node table*/
void createdMemoryTable(){
	int count;

	tableIndexSize=50;
	memoryTable = MALLOC(tableIndexSize * sizeof(struct X3D_Node*));

	/* initialize this to a known state */
	for (count=0; count < tableIndexSize; count++) {
		memoryTable[count] = NULL;
	}
}

/*making table bigger*/
void increaseMemoryTable(){
	int count;
	int oldhigh;

	oldhigh = tableIndexSize;

	
	tableIndexSize*=2;
	memoryTable = REALLOC (memoryTable, tableIndexSize * sizeof(struct X3D_Node*) );

	/* initialize this to a known state */
	for (count=oldhigh; count < tableIndexSize; count++) {
		memoryTable[count] = NULL;
	}
	/* printf("increasing memory table=%d\n",tableIndexSize); */
}


/* sort children - use bubble sort with early exit flag */
/* we use this for z buffer rendering; drawing scene in correct z-buffer order */
static void sortChildren (struct Multi_Node *ch, struct Multi_Node *sortedCh) {
	int i,j;
	int nc;
	int noswitch;
	struct X3D_Node *a, *b, *c;

	/* simple, inefficient bubble sort */
	/* this is a fast sort when nodes are already sorted;
	   may wish to go and "QuickSort" or so on, when nodes
	   move around a lot. (Bubblesort is bad when nodes
	   have to be totally reversed) */

	/* has this changed size? */
	if (ch->n != sortedCh->n) {
		FREE_IF_NZ(sortedCh->p);
		sortedCh->p = MALLOC (sizeof (void *) * ch->n);
	}

	/* copy the nodes over; we will sort the sorted list */
	nc = ch->n;
	memcpy (sortedCh->p,ch->p,sizeof (void *) * nc);
	sortedCh->n = nc;

	#ifdef VERBOSE
	printf ("sc start, %d, chptr %u\n",nc,ch);
	#endif

	if (nc < 2) return;

	for(i=0; i<nc; i++) {
		noswitch = TRUE;
		for (j=(nc-1); j>i; j--) {
			/* printf ("comparing %d %d\n",i,j); */
			a = X3D_NODE(sortedCh->p[j-1]);
			b = X3D_NODE(sortedCh->p[j]);

			/* check to see if a child is NULL - if so, skip it */
			if ((a != NULL) && (b != NULL)) {
				if (a->_dist > b->_dist) {
					/* printf ("have to switch %d %d\n",i,j);  */
					c = a;
					sortedCh->p[j-1] = b;
					sortedCh->p[j] = c;
					noswitch = FALSE;
				}
			}	
		}
		/* did we have a clean run? */
		if (noswitch) {
			break;
		}
	}
	
	#ifdef VERBOSE
	printf ("sortChild returning.\n");
	for(i=0; i<nc; i++) {
		b = sortedCh->p[i];
		printf ("child %d %u %f %s",i,b,b->_dist,stringNodeType(b->_nodeType));
		b = ch->p[i];
		printf (" unsorted %u\n",b);

	}
	#endif
}

/* zero the Visibility flag in all nodes */
void zeroVisibilityFlag(void) {
	struct X3D_Node* node;
	int i;
	int ocnum;

	ocnum=-1;

	
	/* do not bother doing this if the inputThread is active. */
	if (isinputThreadParsing()) return;
 	LOCK_MEMORYTABLE

	/* do we have GL_ARB_occlusion_query, or are we still parsing Textures? */
	if ((OccFailed) || isTextureParsing()) {
		/* if we have textures still loading, display all the nodes, so that the textures actually
		   get put into OpenGL-land. If we are not texture parsing... */
		/* no, we do not have GL_ARB_occlusion_query, just tell every node that it has visible children 
		   and hope that, sometime, the user gets a good computer graphics card */
		for (i=0; i<nextEntry; i++){		
			node = memoryTable[i];	
			node->_renderFlags = node->_renderFlags | VF_hasVisibleChildren;
		}	
	} else {
		/* we do... lets zero the hasVisibleChildren flag */
		for (i=0; i<nextEntry; i++){		
			node = memoryTable[i];		
		
			#ifdef OCCLUSIONVERBOSE
			if (((node->_renderFlags) & VF_hasVisibleChildren) != 0) {
			printf ("zeroVisibility - %d is a %s, flags %x\n",i,stringNodeType(node->_nodeType), (node->_renderFlags) & VF_hasVisibleChildren); 
			}
			#endif

			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_hasVisibleChildren);
	
		}		
	}
	UNLOCK_MEMORYTABLE
}

/* go through the linear list of nodes, and do "special things" for special nodes, like
   Sensitive nodes, Viewpoint nodes, ... */

#define CMD(TTT,YYY) \
	/* printf ("nt %s change %d ichange %d\n",stringNodeType(X3D_NODE(YYY)->_nodeType),X3D_NODE(YYY)->_change, X3D_NODE(YYY)->_ichange); */ \
	if (NODE_NEEDS_COMPILING) compile_Metadata##TTT((struct X3D_Metadata##TTT *) YYY)

#define BEGIN_NODE(thistype) case NODE_##thistype:
#define END_NODE break;

#define SIBLING_SENSITIVE(thistype) \
			/* make Sensitive */ \
			if (((struct X3D_##thistype *)node)->enabled) { \
				nParents = ((struct X3D_##thistype *)node)->_nparents; \
				pp = (((struct X3D_##thistype *)node)->_parents); \
			}  

#define ANCHOR_SENSITIVE(thistype) \
			/* make THIS Sensitive - most nodes make the parents sensitive, Anchors have children...*/ \
			anchorPtr = (struct X3D_Anchor *)node;
#ifdef xxx
			nParents = ((struct X3D_Anchor *)node)->children.n; pp = ((struct X3D_Anchor *)node)->children.p; 
#endif

#ifdef VIEWPOINT
#undef VIEWPOINT /* defined for the EAI,SAI, does not concern us uere */
#endif
#define VIEWPOINT(thistype) \
			setBindPtr = (uintptr_t *) ((uintptr_t)(node) + offsetof (struct X3D_##thistype, set_bind));

#define CHILDREN_NODE(thistype) \
			addChildren = NULL; removeChildren = NULL; \
			if (((struct X3D_##thistype *)node)->addChildren.n > 0) { \
				addChildren = &((struct X3D_##thistype *)node)->addChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->children; \
			} \
			if (((struct X3D_##thistype *)node)->removeChildren.n > 0) { \
				removeChildren = &((struct X3D_##thistype *)node)->removeChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->children; \
			} 

#define CHILDREN_SWITCH_NODE(thistype) \
			addChildren = NULL; removeChildren = NULL; \
			if (((struct X3D_##thistype *)node)->addChildren.n > 0) { \
				addChildren = &((struct X3D_##thistype *)node)->addChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->choice; \
			} \
			if (((struct X3D_##thistype *)node)->removeChildren.n > 0) { \
				removeChildren = &((struct X3D_##thistype *)node)->removeChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->choice; \
			} 

#define CHILDREN_LOD_NODE \
			addChildren = NULL; removeChildren = NULL; \
			if (X3D_LODNODE(node)->addChildren.n > 0) { \
				addChildren = &X3D_LODNODE(node)->addChildren; \
				if (X3D_LODNODE(node)->__isX3D == 0) childrenPtr = &X3D_LODNODE(node)->level; \
				else childrenPtr = &X3D_LODNODE(node)->children; \
			} \
			if (X3D_LODNODE(node)->removeChildren.n > 0) { \
				removeChildren = &X3D_LODNODE(node)->removeChildren; \
				if (X3D_LODNODE(node)->__isX3D == 0) childrenPtr = &X3D_LODNODE(node)->level; \
				else childrenPtr = &X3D_LODNODE(node)->children; \
			}

#define EVIN_AND_FIELD_SAME(thisfield, thistype) \
			if ((((struct X3D_##thistype *)node)->set_##thisfield.n) > 0) { \
				((struct X3D_##thistype *)node)->thisfield.n = 0; \
				FREE_IF_NZ (((struct X3D_##thistype *)node)->thisfield.p); \
				((struct X3D_##thistype *)node)->thisfield.p = ((struct X3D_##thistype *)node)->set_##thisfield.p; \
				((struct X3D_##thistype *)node)->thisfield.n = ((struct X3D_##thistype *)node)->set_##thisfield.n; \
				((struct X3D_##thistype *)node)->set_##thisfield.n = 0; \
				((struct X3D_##thistype *)node)->set_##thisfield.p = NULL; \
			} 

/* just tell the parent (a grouping node) that there is a locally scoped light as a child */
/* do NOT send this up the scenegraph! */
#define LOCAL_LIGHT_PARENT_FLAG \
{ int i; \
	for (i = 0; i < node->_nparents; i++) { \
		struct X3D_Node *n = X3D_NODE(node->_parents[i]); \
		if( n != 0 ) n->_renderFlags = n->_renderFlags | VF_localLight; \
	} \
}



#define  CHECK_MATERIAL_TRANSPARENCY \
	if (((struct X3D_Material *)node)->transparency > 0.0001) { \
		/* printf ("node %d MATERIAL HAS TRANSPARENCY of %f \n", node, ((struct X3D_Material *)node)->transparency); */ \
		update_renderFlag(X3D_NODE(node),VF_Blend);\
		have_transparency = TRUE; \
	}
 
#define CHECK_IMAGETEXTURE_TRANSPARENCY \
	if (isTextureAlpha(((struct X3D_ImageTexture *)node)->__textureTableIndex)) { \
		/* printf ("node %d IMAGETEXTURE HAS TRANSPARENCY\n", node); */ \
		update_renderFlag(X3D_NODE(node),VF_Blend);\
		have_transparency = TRUE; \
	}

#define CHECK_PIXELTEXTURE_TRANSPARENCY \
	if (isTextureAlpha(((struct X3D_PixelTexture *)node)->__textureTableIndex)) { \
		/* printf ("node %d PIXELTEXTURE HAS TRANSPARENCY\n", node); */ \
		update_renderFlag(X3D_NODE(node),VF_Blend);\
		have_transparency = TRUE; \
	}

#define CHECK_MOVIETEXTURE_TRANSPARENCY \
	if (isTextureAlpha(((struct X3D_MovieTexture *)node)->__textureTableIndex)) { \
		/* printf ("node %d MOVIETEXTURE HAS TRANSPARENCY\n", node); */ \
		update_renderFlag(X3D_NODE(node),VF_Blend);\
		have_transparency = TRUE; \
	}


void startOfLoopNodeUpdates(void) {
	struct X3D_Node* node;
	struct X3D_Anchor* anchorPtr;
	void **pp;
	int nParents;
	int i,j;
	uintptr_t *setBindPtr;

	struct Multi_Node *addChildren;
	struct Multi_Node *removeChildren;
	struct Multi_Node *childrenPtr;

	/* initialization */
	addChildren = NULL;
	removeChildren = NULL;
	childrenPtr = NULL;
	pp = NULL;

	/* assume that we do not have any sensitive nodes at all... */
	HaveSensitive = FALSE;
	have_transparency = FALSE;


	/* do not bother doing this if the inputparsing thread is active */
	if (isinputThreadParsing()) return;

	LOCK_MEMORYTABLE

	/* go through the node table, and zero any bits of interest */
	for (i=0; i<nextEntry; i++){		
		node = memoryTable[i];	
		if (node != NULL) {
			/* turn OFF these flags */
			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_Sensitive);
			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_Viewpoint);
			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_localLight);
			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_globalLight);
			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_Blend);
		}
	}

	/* sort the rootNode, if it is Not NULL */
	if (rootNode != NULL) {
		sortChildren (&X3D_GROUP(rootNode)->children, &X3D_GROUP(rootNode)->_sortedChildren);
	}

	/* go through the list of nodes, and "work" on any that need work */
	nParents = 0;
	setBindPtr = NULL;
	childrenPtr = NULL;
	anchorPtr = NULL;

	for (i=0; i<nextEntry; i++){		
		node = memoryTable[i];	
		if (node != NULL) {
			switch (node->_nodeType) {
				BEGIN_NODE(Shape)
					/* send along a "look at me" flag if we are visible, or we should look again */
					if ((X3D_SHAPE(node)->__occludeCheckCount <=0) ||
							(X3D_SHAPE(node)->__visible)) {
						update_renderFlag (X3D_NODE(node),VF_hasVisibleChildren);
						/* printf ("shape occludecounter, pushing visiblechildren flags\n");  */

					}
					X3D_SHAPE(node)->__occludeCheckCount--;
					/* printf ("shape occludecounter %d\n",X3D_SHAPE(node)->__occludeCheckCount); */
				END_NODE

				/* Lights. DirectionalLights are "scope relative", PointLights and
				   SpotLights are transformed */

				BEGIN_NODE(DirectionalLight)
					if (X3D_DIRECTIONALLIGHT(node)->on) {
						if (X3D_DIRECTIONALLIGHT(node)->global) 
							update_renderFlag(node,VF_globalLight);
						else
							LOCAL_LIGHT_PARENT_FLAG
					}
				END_NODE
				BEGIN_NODE(SpotLight)
					if (X3D_SPOTLIGHT(node)->on) {
						if (X3D_SPOTLIGHT(node)->global) 
							update_renderFlag(node,VF_globalLight);
						else
							LOCAL_LIGHT_PARENT_FLAG
					}
				END_NODE
				BEGIN_NODE(PointLight)
					if (X3D_POINTLIGHT(node)->on) {
						if (X3D_POINTLIGHT(node)->global) 
							update_renderFlag(node,VF_globalLight);
						else
							LOCAL_LIGHT_PARENT_FLAG
					}
				END_NODE


				/* some nodes, like Extrusions, have "set_" fields same as normal internal fields,
				   eg, "set_spine" and "spine". Here we just copy the fields over, and remove the
				   "set_" fields. This works for MF* fields ONLY */
				BEGIN_NODE(IndexedLineSet)
					EVIN_AND_FIELD_SAME(colorIndex,IndexedLineSet)
					EVIN_AND_FIELD_SAME(coordIndex,IndexedLineSet)
				END_NODE

				BEGIN_NODE(IndexedTriangleFanSet)
					EVIN_AND_FIELD_SAME(index,IndexedTriangleFanSet)
				END_NODE
				BEGIN_NODE(IndexedTriangleSet)
					EVIN_AND_FIELD_SAME(index,IndexedTriangleSet)
				END_NODE
				BEGIN_NODE(IndexedTriangleStripSet)
					EVIN_AND_FIELD_SAME(index,IndexedTriangleStripSet)
				END_NODE
				BEGIN_NODE(GeoElevationGrid)
					EVIN_AND_FIELD_SAME(height,GeoElevationGrid)
				END_NODE
				BEGIN_NODE(ElevationGrid)
					EVIN_AND_FIELD_SAME(height,ElevationGrid)
				END_NODE
				BEGIN_NODE(Extrusion)
					EVIN_AND_FIELD_SAME(crossSection,Extrusion)
					EVIN_AND_FIELD_SAME(orientation,Extrusion)
					EVIN_AND_FIELD_SAME(scale,Extrusion)
					EVIN_AND_FIELD_SAME(spine,Extrusion)
				END_NODE
				BEGIN_NODE(IndexedFaceSet)
					EVIN_AND_FIELD_SAME(colorIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(coordIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(normalIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(texCoordIndex,IndexedFaceSet)
				END_NODE
/* GeoViewpoint works differently than other nodes - see compile_GeoViewpoint for manipulation of these fields
				BEGIN_NODE(GeoViewpoint)
					EVIN_AND_FIELD_SAME(orientation,GeoViewpoint) 
					EVIN_AND_FIELD_SAME(position,GeoViewpoint)
				END_NODE
*/
	
				/* get ready to mark these nodes as Mouse Sensitive */
				BEGIN_NODE(PlaneSensor) SIBLING_SENSITIVE(PlaneSensor) END_NODE
				BEGIN_NODE(SphereSensor) SIBLING_SENSITIVE(SphereSensor) END_NODE
				BEGIN_NODE(CylinderSensor) SIBLING_SENSITIVE(CylinderSensor) END_NODE
				BEGIN_NODE(TouchSensor) SIBLING_SENSITIVE(TouchSensor) END_NODE
				BEGIN_NODE(GeoTouchSensor) SIBLING_SENSITIVE(GeoTouchSensor) END_NODE
	
				/* Anchor is Mouse Sensitive, AND has Children nodes */
				BEGIN_NODE(Anchor)
					sortChildren(&X3D_ANCHOR(node)->children,&X3D_ANCHOR(node)->_sortedChildren);
					propagateExtent(X3D_NODE(node));
					ANCHOR_SENSITIVE(Anchor)
					CHILDREN_NODE(Anchor)
				END_NODE
				
				/* maybe this is the current Viewpoint? */
				BEGIN_NODE(Viewpoint) VIEWPOINT(Viewpoint) END_NODE
				BEGIN_NODE(GeoViewpoint) VIEWPOINT(GeoViewpoint) END_NODE
	
				BEGIN_NODE(StaticGroup)
					/* we should probably not do this, but... */
					sortChildren(&X3D_STATICGROUP(node)->children,&X3D_STATICGROUP(node)->_sortedChildren);
					propagateExtent(X3D_NODE(node));
				END_NODE


				/* does this one possibly have add/removeChildren? */
				BEGIN_NODE(Group) 
					sortChildren(&X3D_GROUP(node)->children,&X3D_GROUP(node)->_sortedChildren);

					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(Group) 
				END_NODE

				BEGIN_NODE(Inline) 
					sortChildren (&X3D_INLINE(node)->__children,&X3D_INLINE(node)->_sortedChildren);
					propagateExtent(X3D_NODE(node));
				END_NODE

				BEGIN_NODE(Transform) 
					sortChildren(&X3D_TRANSFORM(node)->children,&X3D_TRANSFORM(node)->_sortedChildren);
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(Transform) 
				END_NODE

				BEGIN_NODE(NurbsGroup) 
					CHILDREN_NODE(NurbsGroup) 
				END_NODE

				BEGIN_NODE(Contour2D) 
					CHILDREN_NODE(Contour2D) 
				END_NODE

				BEGIN_NODE(HAnimSite) 
					CHILDREN_NODE(HAnimSite) 
				END_NODE

				BEGIN_NODE(HAnimSegment) 
					CHILDREN_NODE(HAnimSegment) 
				END_NODE

				BEGIN_NODE(HAnimJoint) 
					CHILDREN_NODE(HAnimJoint) 
				END_NODE

				BEGIN_NODE(Billboard) 
					sortChildren (&X3D_BILLBOARD(node)->children,&X3D_BILLBOARD(node)->_sortedChildren);
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(Billboard) 
                			update_renderFlag(node,VF_Proximity);
				END_NODE

				BEGIN_NODE(Collision) 
					sortChildren (&X3D_COLLISION(node)->children,&X3D_COLLISION(node)->_sortedChildren);
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(Collision) 
				END_NODE

				BEGIN_NODE(Switch) 
					propagateExtent(X3D_NODE(node));
					CHILDREN_SWITCH_NODE(Switch) 
				END_NODE

				BEGIN_NODE(LOD) 
					propagateExtent(X3D_NODE(node));
					CHILDREN_LOD_NODE 
                			update_renderFlag(node,VF_Proximity);
				END_NODE

				/* Material - transparency of materials */
				BEGIN_NODE(Material) CHECK_MATERIAL_TRANSPARENCY END_NODE

				/* Textures - check transparency  */
				BEGIN_NODE(ImageTexture) CHECK_IMAGETEXTURE_TRANSPARENCY END_NODE
				BEGIN_NODE(PixelTexture) CHECK_PIXELTEXTURE_TRANSPARENCY END_NODE
				BEGIN_NODE(MovieTexture) CHECK_MOVIETEXTURE_TRANSPARENCY END_NODE

				/* VisibilitySensor needs its own flag sent up the chain */
				BEGIN_NODE (VisibilitySensor)
					/* send along a "look at me" flag if we are visible, or we should look again */
					if ((X3D_VISIBILITYSENSOR(node)->__occludeCheckCount <=0) ||
							(X3D_VISIBILITYSENSOR(node)->__visible)) {
						update_renderFlag (X3D_NODE(node),VF_hasVisibleChildren);
						/* printf ("vis occludecounter, pushing visiblechildren flags\n"); */

					}
					X3D_VISIBILITYSENSOR(node)->__occludeCheckCount--;
					/* VisibilitySensors have a transparent bounding box we have to render */
                			update_renderFlag(node,VF_Blend);
				END_NODE

				/* ProximitySensor needs its own flag sent up the chain */
				BEGIN_NODE (ProximitySensor)
                			if (X3D_PROXIMITYSENSOR(node)->enabled) update_renderFlag(node,VF_Proximity);
				END_NODE

				/* GeoProximitySensor needs its own flag sent up the chain */
				BEGIN_NODE (GeoProximitySensor)
                			if (X3D_GEOPROXIMITYSENSOR(node)->enabled) update_renderFlag(node,VF_Proximity);
				END_NODE

				/* GeoLOD needs its own flag sent up the chain, and it has to push extent up, too */
				BEGIN_NODE (GeoLOD)
					if (!(NODE_NEEDS_COMPILING)) {
						handle_GeoLODRange(X3D_GEOLOD(node));
					}
                			/* update_renderFlag(node,VF_Proximity); */
					propagateExtent(X3D_NODE(node));
				END_NODE

				BEGIN_NODE (GeoTransform)
					sortChildren(&X3D_GEOTRANSFORM(node)->children,&X3D_GEOTRANSFORM(node)->_sortedChildren);
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(GeoTransform) 
				END_NODE

				BEGIN_NODE (GeoLocation)
					sortChildren(&X3D_GEOLOCATION(node)->children,&X3D_GEOLOCATION(node)->_sortedChildren);
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(GeoLocation) 
				END_NODE

				BEGIN_NODE(MetadataSFBool) CMD(SFBool,node); END_NODE
				BEGIN_NODE(MetadataSFFloat) CMD(SFFloat,node); END_NODE
				BEGIN_NODE(MetadataMFFloat) CMD(MFFloat,node); END_NODE
				BEGIN_NODE(MetadataSFRotation) CMD(SFRotation,node); END_NODE
				BEGIN_NODE(MetadataMFRotation) CMD(MFRotation,node); END_NODE
				BEGIN_NODE(MetadataSFVec3f) CMD(SFVec3f,node); END_NODE
				BEGIN_NODE(MetadataMFVec3f) CMD(MFVec3f,node); END_NODE
				BEGIN_NODE(MetadataMFBool) CMD(MFBool,node); END_NODE
				BEGIN_NODE(MetadataSFInt32) CMD(SFInt32,node); END_NODE
				BEGIN_NODE(MetadataMFInt32) CMD(MFInt32,node); END_NODE
				BEGIN_NODE(MetadataSFNode) CMD(SFNode,node); END_NODE
				BEGIN_NODE(MetadataMFNode) CMD(MFNode,node); END_NODE
				BEGIN_NODE(MetadataSFColor) CMD(SFColor,node); END_NODE
				BEGIN_NODE(MetadataMFColor) CMD(MFColor,node); END_NODE
				BEGIN_NODE(MetadataSFColorRGBA) CMD(SFColorRGBA,node); END_NODE
				BEGIN_NODE(MetadataMFColorRGBA) CMD(MFColorRGBA,node); END_NODE
				BEGIN_NODE(MetadataSFTime) CMD(SFTime,node); END_NODE
				BEGIN_NODE(MetadataMFTime) CMD(MFTime,node); END_NODE
				BEGIN_NODE(MetadataSFString) CMD(SFString,node); END_NODE
				BEGIN_NODE(MetadataMFString) CMD(MFString,node); END_NODE
				BEGIN_NODE(MetadataSFVec2f) CMD(SFVec2f,node); END_NODE
				BEGIN_NODE(MetadataMFVec2f) CMD(MFVec2f,node); END_NODE
				BEGIN_NODE(MetadataSFImage) CMD(SFImage,node); END_NODE
				BEGIN_NODE(MetadataSFVec3d) CMD(SFVec3d,node); END_NODE
				BEGIN_NODE(MetadataMFVec3d) CMD(MFVec3d,node); END_NODE
				BEGIN_NODE(MetadataSFDouble) CMD(SFDouble,node); END_NODE
				BEGIN_NODE(MetadataMFDouble) CMD(MFDouble,node); END_NODE
				BEGIN_NODE(MetadataSFMatrix3f) CMD(SFMatrix3f,node); END_NODE
				BEGIN_NODE(MetadataMFMatrix3f) CMD(MFMatrix3f,node); END_NODE
				BEGIN_NODE(MetadataSFMatrix3d) CMD(SFMatrix3d,node); END_NODE
				BEGIN_NODE(MetadataMFMatrix3d) CMD(MFMatrix3d,node); END_NODE
				BEGIN_NODE(MetadataSFMatrix4f) CMD(SFMatrix4f,node); END_NODE
				BEGIN_NODE(MetadataMFMatrix4f) CMD(MFMatrix4f,node); END_NODE
				BEGIN_NODE(MetadataSFMatrix4d) CMD(SFMatrix4d,node); END_NODE
				BEGIN_NODE(MetadataMFMatrix4d) CMD(MFMatrix4d,node); END_NODE
				BEGIN_NODE(MetadataSFVec2d) CMD(SFVec2d,node); END_NODE
				BEGIN_NODE(MetadataMFVec2d) CMD(MFVec2d,node); END_NODE
				BEGIN_NODE(MetadataSFVec4f) CMD(SFVec4f,node); END_NODE
				BEGIN_NODE(MetadataMFVec4f) CMD(MFVec4f,node); END_NODE
				BEGIN_NODE(MetadataSFVec4d) CMD(SFVec4d,node); END_NODE
				BEGIN_NODE(MetadataMFVec4d) CMD(MFVec4d,node); END_NODE

				/* VRML1 Separator node; we do a bare bones implementation; always assume there are 
					lights, geometry, and viewpoints here. */
				BEGIN_NODE(VRML1_Separator) 
					sortChildren(&VRML1_SEPARATOR(node)->VRML1children,&VRML1_SEPARATOR(node)->_sortedChildren);
					propagateExtent(X3D_NODE(node));
					update_renderFlag(X3D_NODE(node),VF_localLight|VF_Viewpoint|VF_Geom|VF_hasVisibleChildren);
				END_NODE
			}
		}

		/* now, act on this node  for Sensitive nodes. here we tell the PARENTS that they are sensitive */
		if (nParents != 0) {
			for (j=0; j<nParents; j++) {
				struct X3D_Node *n = X3D_NODE(pp[j]);
				n->_renderFlags = n->_renderFlags  | VF_Sensitive;
			}

			/* tell mainloop that we have to do a sensitive pass now */
			HaveSensitive = TRUE;
			nParents = 0;
		}

		/* Anchor nodes are slightly different than sibling-sensitive nodes */
		if (anchorPtr != NULL) {
			anchorPtr->_renderFlags = anchorPtr->_renderFlags  | VF_Sensitive;

			/* tell mainloop that we have to do a sensitive pass now */
			HaveSensitive = TRUE;
			anchorPtr = NULL;
		}

		/* do BINDING of Viewpoint Nodes */
		if (setBindPtr != NULL) {
			/* check the set_bind eventin to see if it is TRUE or FALSE */
			if (*setBindPtr < 100) {
				/* printf ("Found a vp to modify %d\n",node); */
				/* up_vector is reset after a bind */
				if (*setBindPtr==1) reset_upvector();
				bind_node ((void *)node, &viewpoint_tos,&viewpoint_stack[0]);
			}
			setBindPtr = NULL;
		}

		/* this node possibly has to do add/remove children? */
		if (childrenPtr != NULL) {
			if (addChildren != NULL) {
				AddRemoveChildren(node,childrenPtr,(uintptr_t *) addChildren->p,addChildren->n,1,__FILE__,__LINE__);
				addChildren->n=0;
			}
			if (removeChildren != NULL) {
				AddRemoveChildren(node,childrenPtr,(uintptr_t *) removeChildren->p,removeChildren->n,2,__FILE__,__LINE__);
				removeChildren->n=0;
			}
			childrenPtr = NULL;
		}
	}



	UNLOCK_MEMORYTABLE

	/* now, we can go and tell the grouping nodes which ones are the lucky ones that contain the current Viewpoint node */
	if (viewpoint_stack[viewpoint_tos] != 0) {
		update_renderFlag(X3D_NODE(viewpoint_stack[viewpoint_tos]), VF_Viewpoint);
		calculateNearFarplanes(X3D_NODE(viewpoint_stack[viewpoint_tos]));
	} else {
		/* keep these at the defaults, if no viewpoint is present. */
		nearPlane = DEFAULT_NEARPLANE;
		farPlane = DEFAULT_FARPLANE;
		backgroundPlane = DEFAULT_BACKGROUNDPLANE;
	}
}

/*delete node created*/
void kill_X3DNodes(void){
	int i=0;
	int j=0;
	uintptr_t *fieldOffsetsPtr;
	char * fieldPtr;
	struct X3D_Node* structptr;
	struct Multi_Float* MFloat;
	struct Multi_Rotation* MRotation;
	struct Multi_Vec3f* MVec3f;
	struct Multi_Bool* Mbool;
	struct Multi_Int32* MInt32;
	struct Multi_Node* MNode;
	struct Multi_Color* MColor;
	struct Multi_ColorRGBA* MColorRGBA;
	struct Multi_Time* MTime;
	struct Multi_String* MString;
	struct Multi_Vec2f* MVec2f;
	uintptr_t * VPtr;
	struct Uni_String *MyS;

	LOCK_MEMORYTABLE
	/*go thru all node until table is empty*/
	for (i=0; i<nextEntry; i++){		
		structptr = memoryTable[i];		

		#ifdef VERBOSE
		printf("Node pointer	= %u entry %d of %d ",structptr,i,nextEntry);
		printf (" number of parents %d ", structptr->_nparents);
		printf("Node Type	= %s\n",stringNodeType(structptr->_nodeType));  
		#endif

		/* kill any parents that may exist. */
		FREE_IF_NZ (structptr->_parents);

		fieldOffsetsPtr = (uintptr_t *) NODE_OFFSETS[structptr->_nodeType];
		/*go thru all field*/				
		while (*fieldOffsetsPtr != -1) {
			fieldPtr=(char*)structptr+(*(fieldOffsetsPtr+1));
			#ifdef VERBOSE
			printf ("looking at field %s type %s\n",FIELDNAMES[*fieldOffsetsPtr],FIELDTYPES[*(fieldOffsetsPtr+2)]); 
			#endif

			/* some fields we skip, as the pointers are duplicated, and we CAN NOT free both */
			if (*fieldOffsetsPtr == FIELDNAMES_setValue) 
				break; /* can be a duplicate SF/MFNode pointer */
		
			if (*fieldOffsetsPtr == FIELDNAMES_valueChanged) 
				break; /* can be a duplicate SF/MFNode pointer */
		

			if (*fieldOffsetsPtr == FIELDNAMES___oldmetadata) 
				break; /* can be a duplicate SFNode pointer */
		
			if (*fieldOffsetsPtr == FIELDNAMES___lastParent) 
				break; /* can be a duplicate SFNode pointer - field only in NODE_TextureCoordinate */
		
			if (*fieldOffsetsPtr == FIELDNAMES_FreeWRL__protoDef) 
				break; /* can be a duplicate SFNode pointer - field only in NODE_Group */
		
			if (*fieldOffsetsPtr == FIELDNAMES__selected) 
				break; /* can be a duplicate SFNode pointer - field only in NODE_LOD and NODE_GeoLOD */

			if (*fieldOffsetsPtr == FIELDNAMES___oldChildren) 
				break; /* can be a duplicate SFNode pointer - field only in NODE_LOD and NODE_GeoLOD */

			if (*fieldOffsetsPtr == FIELDNAMES___oldMFString) 
				break; 

			if (*fieldOffsetsPtr == FIELDNAMES___oldSFString) 
				break; 

			if (*fieldOffsetsPtr == FIELDNAMES___oldKeyPtr) 
				break; /* used for seeing if interpolator values change */

			if (*fieldOffsetsPtr == FIELDNAMES___oldKeyValuePtr) 
				break; /* used for seeing if interpolator values change */

			if (*fieldOffsetsPtr == FIELDNAMES___shaderIDS) {
				struct X3D_ComposedShader *cps = (struct X3D_ComposedShader *) structptr;
				if ((cps->_nodeType == NODE_ComposedShader) || (cps->_nodeType == NODE_ProgramShader)) {
#ifdef GL_VERSION_2_0
					if (cps->__shaderIDS.p != NULL) {
						glDeleteProgram((GLuint) cps->__shaderIDS.p[0]);
						FREE_IF_NZ(cps->__shaderIDS.p);
						cps->__shaderIDS.n=0;
					}
#endif

				} else {
					ConsoleMessage ("error destroying shaderIDS on kill");
				}
			}

			/* GeoLOD nodes, the children field exports either the rootNode, or the list of child nodes */
			if (structptr->_nodeType == NODE_GeoLOD) {
				if (*fieldOffsetsPtr == FIELDNAMES_children) break;
			}
		
			/* nope, not a special field, lets just get rid of it as best we can */
			switch(*(fieldOffsetsPtr+2)){
				case FIELDTYPE_MFFloat:
					MFloat=(struct Multi_Float *)fieldPtr;
					MFloat->n=0;
					FREE_IF_NZ(MFloat->p);
					break;
				case FIELDTYPE_MFRotation:
					MRotation=(struct Multi_Rotation *)fieldPtr;
					MRotation->n=0;
					FREE_IF_NZ(MRotation->p);
					break;
				case FIELDTYPE_MFVec3f:
					MVec3f=(struct Multi_Vec3f *)fieldPtr;
					MVec3f->n=0;
					FREE_IF_NZ(MVec3f->p);
					break;
				case FIELDTYPE_MFBool:
					Mbool=(struct Multi_Bool *)fieldPtr;
					Mbool->n=0;
					FREE_IF_NZ(Mbool->p);
					break;
				case FIELDTYPE_MFInt32:
					MInt32=(struct Multi_Int32 *)fieldPtr;
					MInt32->n=0;
					FREE_IF_NZ(MInt32->p);
					break;
				case FIELDTYPE_MFNode:
					MNode=(struct Multi_Node *)fieldPtr;
					#ifdef VERBOSE
					/* verify node structure. Each child should point back to me. */
					{
						int i;
						struct X3D_Node *tp;
						for (i=0; i<MNode->n; i++) {
							tp = MNode->p[i];
							printf ("	MNode field has child %u\n",tp);
							printf ("	ct %s\n",stringNodeType(tp->_nodeType));
						}
					}	
					#endif
					MNode->n=0;
					FREE_IF_NZ(MNode->p);
					break;

				case FIELDTYPE_MFColor:
					MColor=(struct Multi_Color *)fieldPtr;
					MColor->n=0;
					FREE_IF_NZ(MColor->p);
					break;
				case FIELDTYPE_MFColorRGBA:
					MColorRGBA=(struct Multi_ColorRGBA *)fieldPtr;
					MColorRGBA->n=0;
					FREE_IF_NZ(MColorRGBA->p);
					break;
				case FIELDTYPE_MFTime:
					MTime=(struct Multi_Time *)fieldPtr;
					MTime->n=0;
					FREE_IF_NZ(MTime->p);
					break;
				case FIELDTYPE_MFString: 
					MString=(struct Multi_String *)fieldPtr;
					{
					struct Uni_String* ustr;
					for (j=0; j<MString->n; j++) {
						ustr=MString->p[j];
						ustr->len=0;
						ustr->touched=0;
						FREE_IF_NZ(ustr->strptr);
					}
					MString->n=0;
					FREE_IF_NZ(MString->p);
					}
					break;
				case FIELDTYPE_MFVec2f:
					MVec2f=(struct Multi_Vec2f *)fieldPtr;
					MVec2f->n=0;
					FREE_IF_NZ(MVec2f->p);
					break;
				case FIELDTYPE_FreeWRLPTR:
					VPtr = (uintptr_t *) fieldPtr;
					VPtr = (uintptr_t *) (*VPtr);
					FREE_IF_NZ(VPtr);
					break;
				case FIELDTYPE_SFString:
					VPtr = (uintptr_t *) fieldPtr;
					MyS = (struct Uni_String *) *VPtr;
					MyS->len = 0;
					FREE_IF_NZ(MyS->strptr);
					FREE_IF_NZ(MyS);
					break;
					
				default:; /* do nothing - field not malloc'd */
			}
			fieldOffsetsPtr+=5;	
		}
		FREE_IF_NZ(memoryTable[i]);
		memoryTable[i]=NULL;
	}
	FREE_IF_NZ(memoryTable);
	memoryTable=NULL;
	tableIndexSize=INT_ID_UNDEFINED;
	nextEntry=0;
	UNLOCK_MEMORYTABLE
}
