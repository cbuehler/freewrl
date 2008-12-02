/*
=INSERT_TEMPLATE_HERE=

$Id: headers.h,v 1.3 2008/11/27 01:51:43 couannette Exp $

Global includes.

*/

#ifndef __FREEX3D_HEADERS_H__
#define __FREEX3D_HEADERS_H__


/**
 * in utils.c
 */
char *BrowserName;
const char* freex3d_get_browser_program();

/**
 * in InputFunctions.c
 */
int dirExists(const char *dir);
char* makeFontDirectory();
char *readInputString(char *fn);


/* see if an inputOnly "set_" field has changed */
#define IO_FLOAT -2335549.0

void *freewrlMalloc(int line, char *file, size_t sz);
void *freewrlRealloc (int line, char *file, void *ptr, size_t size);
void *freewrlStrdup (int line, char *file, char *str);
#define MALLOC(sz) FWMALLOC (__LINE__,__FILE__,sz)
#define FWMALLOC(l,f,sz) freewrlMalloc(l,f,sz)
#define REALLOC(a,b) freewrlRealloc(__LINE__,__FILE__,a,b)
#define STRDUP(a) freewrlStrdup(__LINE__,__FILE__,a)
#ifdef DEBUG_MALLOC
	/* free a malloc'd pointer */
	void freewrlFree(int line, char *file, void *a);
	#define FREE_IF_NZ(a) if(a) {freewrlFree(__LINE__,__FILE__,a); a = 0;}

#else 
	#define FREE_IF_NZ(a) if(a) {free(a); a = 0;}
#endif

#define UNLINK(fdd) {/* printf ("unlinking %s at %s:%d\n",fdd,__FILE__,__LINE__); */ unlink (fdd); }

/* children fields path optimizations */
#define CHILDREN_COUNT int nc = node->children.n;
#define RETURN_FROM_CHILD_IF_NOT_FOR_ME \
        /* any children at all? */ \
        if (nc==0) return;      \
        /* should we go down here? */ \
        /* printf ("Group, rb %x VF_B %x, rg  %x VF_G %x\n",render_blend, VF_Blend, render_geom, VF_Geom); */ \
        if (render_blend == VF_Blend) \
                if ((node->_renderFlags & VF_Blend) != VF_Blend) { \
                        return; \
                } \
        if (render_proximity == VF_Proximity) \
                if ((node->_renderFlags & VF_Proximity) != VF_Proximity)  { \
                        return; \
                } \




#undef DEBUG_JAVASCRIPT_PROPERTY
#ifdef DEBUG_JAVASCRIPT_PROPERTY
#define JS_GET_PROPERTY_STUB js_GetPropertyDebug
#define JS_SET_PROPERTY_STUB1 js_SetPropertyDebug1
#define JS_SET_PROPERTY_STUB2 js_SetPropertyDebug2 
#define JS_SET_PROPERTY_STUB3 js_SetPropertyDebug3 
#define JS_SET_PROPERTY_STUB4 js_SetPropertyDebug4 
#define JS_SET_PROPERTY_STUB5 js_SetPropertyDebug5 
#define JS_SET_PROPERTY_STUB6 js_SetPropertyDebug6 
#define JS_SET_PROPERTY_STUB7 js_SetPropertyDebug7 
#else
#define JS_GET_PROPERTY_STUB JS_PropertyStub
#define JS_SET_PROPERTY_STUB1 JS_PropertyStub
#define JS_SET_PROPERTY_STUB2 JS_PropertyStub
#define JS_SET_PROPERTY_STUB3 JS_PropertyStub
#define JS_SET_PROPERTY_STUB4 JS_PropertyStub
#define JS_SET_PROPERTY_STUB5 JS_PropertyStub
#define JS_SET_PROPERTY_STUB6 JS_PropertyStub
#define JS_SET_PROPERTY_STUB7 JS_PropertyStub
#endif

#define ID_UNDEFINED -1

/* stop the display thread. Used (when this comment was made) by the OSX Safari plugin; keeps
most things around, just stops display thread, when the user exits a world. */
#define STOP_DISPLAY_THREAD \
        if (DispThrd != NULL) { \
                quitThread = TRUE; \
                pthread_join(DispThrd,NULL); \
                DispThrd = NULL; \
        }


typedef struct _CRnodeStruct {
        struct X3D_Node *routeToNode;
        unsigned int foffset;
} CRnodeStruct;

/* Size of static array */
#define ARR_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

/* Some stuff for routing */
#define FROM_SCRIPT 1
#define TO_SCRIPT 2
#define SCRIPT_TO_SCRIPT 3

/* Helper to get size of a struct's memer */
#define sizeof_member(str, var) \
 sizeof(((str*)NULL)->var)

/* C routes */
#define MAXJSVARIABLELENGTH 25	/* variable name length can be this long... */

struct CRStruct {
        struct X3D_Node*  routeFromNode;
        uintptr_t fnptr;
        unsigned int tonode_count;
        CRnodeStruct *tonodes;
        int     isActive;
        int     len;
        void    (*interpptr)(void *); /* pointer to an interpolator to run */
        int     direction_flag; /* if non-zero indicates script in/out,
                                                   proto in/out */
        int     extra;          /* used to pass a parameter (eg, 1 = addChildren..) */
};
struct CRjsnameStruct {
        int     	type;
        char    	name[MAXJSVARIABLELENGTH];
	uintptr_t 	eventInFunction;		/* compiled javascript function... if it is required */
};


extern struct CRjsnameStruct *JSparamnames;
extern struct CRStruct *CRoutes;
extern int jsnameindex;
extern int MAXJSparamNames;

extern char *BrowserName;
extern char *BrowserFullPath;

/* To allow BOOL for boolean values */
#define BOOL	int

/* multi-threaded OpenGL contexts - works on OS X, kind of ok on Linux, but
   blows plugins out of the water, because of the XLib threaded call in FrontEnd
   not working that well... */
#ifdef AQUA
	#define DO_MULTI_OPENGL_THREADS
#endif

/* display the BoundingBoxen */
#undef  DISPLAYBOUNDINGBOX

/* rendering constants used in SceneGraph, etc. */
#define VF_Viewpoint 				0x0001
#define VF_Geom 				0x0002
#define VF_DirectionalLight			0x0004 
#define VF_Sensitive 				0x0008
#define VF_Blend 				0x0010
#define VF_Proximity 				0x0020
#define VF_Collision 				0x0040
#define VF_hasVisibleChildren 			0x0100
#define VF_otherLight				0x0800 

/* for z depth buffer calculations */
#define DEFAULT_NEARPLANE 0.1
#define xxDEFAULT_FARPLANE 21000.0
#define DEFAULT_FARPLANE 210.0
extern double geoHeightinZAxis;


/* compile simple nodes (eg, Cone, LineSet) into an internal format. Check out
   CompileC in VRMLRend.pm, and look for compile_* functions in code. General
   meshes are rendered using the PolyRep scheme, which also compiles into OpenGL 
   calls, using the PolyRep (and, stream_PolyRep) methodology */

int isShapeCompilerParsing(void);
void compile_polyrep(void *node, void *coord, void *color, void *normal, void *texCoord);
#define COMPILE_POLY_IF_REQUIRED(a,b,c,d) \
                if(!node->_intern || node->_change != ((struct X3D_PolyRep *)node->_intern)->irep_change) { \
                        compileNode ((void *)compile_polyrep, node, a,b,c,d); \
		} \
		if (!node->_intern) return;

#define COMPILE_IF_REQUIRED { struct X3D_Virt *v; \
	if (node->_ichange != node->_change) { \
		/* printf ("COMP %d %d\n",node->_ichange, node->_change); */ \
		v = *(struct X3D_Virt **)node; \
		if (v->compile) { \
			compileNode (v->compile, (void *)node, NULL, NULL, NULL, NULL); \
		} else {printf ("huh - have COMPIFREQD, but v->compile null for %s\n",stringNodeType(node->_nodeType));} \
		} \
		if (node->_ichange == 0) return; \
	}

/* same as COMPILE_IF_REQUIRED, but passes in node name */
#define COMPILE_IF_REQUIRED2(node) { struct X3D_Virt *v; \
	if (node->_ichange != node->_change) { \
	/* printf ("COMP %d %d\n",node->_ichange, node->_change); */ \
		v = *(struct X3D_Virt **)node; \
		if (v->compile) { \
			compileNode (v->compile, (void *)node, NULL, NULL, NULL, NULL); \
		} else {printf ("huh - have COMPIFREQD, but v->compile null for %s\n",stringNodeType(node->_nodeType));} \
		} \
		if (node->_ichange == 0) return; \
	}


/* convert a PROTO node (which will be a Group node) into a node. eg, for Materials  - this is a possible child
node for ANY node that takes something other than a Group */
#define POSSIBLE_PROTO_EXPANSION(inNode,outNode) \
	if (inNode == NULL) outNode = NULL; \
	else {if (X3D_NODE(inNode)->_nodeType == NODE_Group) { \
		if (X3D_GROUP(inNode)->children.n>0) { \
			outNode = X3D_GROUP(inNode)->children.p[0]; \
		} else outNode = NULL; \
	} else outNode = inNode; };


#define RENDER_MATERIAL_SUBNODES(which) \
	{ void *tmpN;   \
		POSSIBLE_PROTO_EXPANSION(which,tmpN) \
       		if(tmpN) { \
			render_node(tmpN); \
       		} else { \
			/* no material, so just colour the following shape */ \
	       		/* Spec says to disable lighting and set coloUr to 1,1,1 */ \
	       		LIGHTING_OFF  \
       			glColor3f(1,1,1); \
 \
			/* tell the rendering passes that this is just "normal" */ \
			last_texture_type = NOTEXTURE; \
			/* same with global_transparency */ \
			global_transparency=0.99999; \
		} \
	}

#define MARK_NODE_COMPILED node->_ichange = node->_change;
#define NODE_NEEDS_COMPILING (node->_ichange != node->_change)
/* end of compile simple nodes code */


#define MARK_SFFLOAT_INOUT_EVENT(good,save,offset) \
        if (!APPROX(good,save)) { \
                MARK_EVENT(X3D_NODE(node), offset);\
                save = good; \
        }

#define MARK_SFTIME_INOUT_EVENT(good,save,offset) \
        if (!APPROX(good,save)) { \
                MARK_EVENT(X3D_NODE(node), offset);\
                save = good; \
        }

#define MARK_SFINT32_INOUT_EVENT(good,save,offset) \
        if (good!=save) { \
                MARK_EVENT(X3D_NODE(node), offset);\
                save = good; \
        }

#define MARK_SFBOOL_INOUT_EVENT(good,save,offset) \
        if (good!=save) { \
                MARK_EVENT(X3D_NODE(node), offset);\
                save = good; \
        }

#define MARK_SFSTRING_INOUT_EVENT(good,save,offset) \
        if (good->strptr!=save->strptr) { \
                MARK_EVENT(X3D_NODE(node), offset);\
                save->strptr = good->strptr; \
        }

#define MARK_MFSTRING_INOUT_EVENT(good,save,offset) \
	/* assumes that the good pointer has been updated */ \
	if (good.p != save.p) { \
                MARK_EVENT(X3D_NODE(node), offset);\
		save.n = good.n; \
		save.p = good.p; \
        }

#define MARK_SFVEC3F_INOUT_EVENT(good,save,offset) \
        if ((!APPROX(good.c[0],save.c[0])) || (!APPROX(good.c[1],save.c[1])) || (!APPROX(good.c[2],save.c[2]))) { \
                MARK_EVENT(X3D_NODE(node), offset);\
                memcpy (&save.c, &good.c, sizeof (struct SFColor));\
        }

#define MARK_SFVEC3D_INOUT_EVENT(good,save,offset) \
        if ((!APPROX(good.c[0],save.c[0])) || (!APPROX(good.c[1],save.c[1])) || (!APPROX(good.c[2],save.c[2]))) { \
                MARK_EVENT(X3D_NODE(node), offset);\
                memcpy (&save.c, &good.c, sizeof (struct SFVec3d));\
        }


/* make sure that the SFNODE "save" field is not deleted when scenegraph is removed (dup pointer problem) */
#define MARK_SFNODE_INOUT_EVENT(good,save,offset) \
        if (good != save) { \
                MARK_EVENT(X3D_NODE(node), offset);\
                save = good; \
        }


#define MARK_MFNODE_INOUT_EVENT(good,save,offset) \
	/* assumes that the good pointer has been updated */ \
	if (good.p != save.p) { \
                MARK_EVENT(X3D_NODE(node), offset);\
		save.n = good.n; \
		save.p = good.p; \
        }

/* for deciding on using set_ SF fields, with nodes with explicit "set_" fields...  note that MF fields are handled by
the EVIN_AND_FIELD_SAME MACRO */

#define USE_SET_SFVEC3D_IF_CHANGED(setField,regField) \
if (!APPROX (node->setField.c[0],node->regField.c[0]) || \
        !APPROX(node->setField.c[1],node->regField.c[1]) || \
        !APPROX(node->setField.c[2],node->regField.c[2]) ) { \
        /* now, is the setField at our default value??  if not, we just use the regField */ \
        if (APPROX(node->setField.c[0], IO_FLOAT) && APPROX(node->setField.c[1],IO_FLOAT) && APPROX(node->setField.c[2],IO_FLOAT)) { \
		/* printf ("just use regField\n"); */ \
        } else { \
		 /* printf ("use the setField as the real poistion field\n"); */ \
        	memcpy (node->regField.c, node->setField.c, sizeof (struct SFVec3d)); \
	} \
}

#define USE_SET_SFROTATION_IF_CHANGED(setField,regField) \
if (!APPROX (node->setField.r[0],node->regField.r[0]) || \
        !APPROX(node->setField.r[1],node->regField.r[1]) || \
        !APPROX(node->setField.r[2],node->regField.r[2]) || \
        !APPROX(node->setField.r[3],node->regField.r[3]) ) { \
        /* now, is the setField at our default value??  if not, we just use the regField */ \
        if (APPROX(node->setField.r[0], IO_FLOAT) && APPROX(node->setField.r[1],IO_FLOAT) && APPROX(node->setField.r[2],IO_FLOAT) && APPROX(node->setField.r[3],IO_FLOAT)) { \
		/* printf ("just use SFRotation regField\n"); */ \
        } else { \
		/* printf ("use the setField SFRotation as the real poistion field\n");  */ \
        	memcpy (node->regField.r, node->setField.r, sizeof (struct SFRotation)); \
	} \
}




int find_key (int kin, float frac, float *keys);
void startOfLoopNodeUpdates(void);
void OcclusionCulling (void);
void OcclusionStartofEventLoop(void);
extern int HaveSensitive;
void zeroVisibilityFlag(void);
void setField_fromJavascript (struct X3D_Node *ptr, char *field, char *value);
unsigned int setField_FromEAI (char *ptr);
void setField_javascriptEventOut(struct X3D_Node  *tn,unsigned int tptr, int fieldType, unsigned len, int extraData, uintptr_t mycx);

extern char *GL_VEN;
extern char *GL_VER;
extern char *GL_REN;

/* do we have GL Occlusion Culling? */
#ifdef AQUA
	#define OCCLUSION
        #define VISIBILITYOCCLUSION
        #define SHAPEOCCLUSION
        #define glGenQueries(a,b) glGenQueriesARB(a,b)
        #define glDeleteQueries(a,b) glDeleteQueriesARB(a,b)
#else
	/* on Linux, test to see if we have this defined. */
	#ifdef HAVE_GL_QUERIES_ARB
		#define OCCLUSION
		#define VISIBILITYOCCLUSION
		#define SHAPEOCCLUSION
		#define glGenQueries(a,b) glGenQueriesARB(a,b)
		#define glDeleteQueries(a,b) glDeleteQueriesARB(a,b)
	#else 
		#undef OCCLUSION
		#undef VISIBILITYOCCLUSION
		#undef SHAPEOCCLUSION
		#define glGenQueries(a,b)
		#define glDeleteQueries(a,b)
	#endif
#endif

#define glIsQuery(a) glIsQueryARB(a)
#define glBeginQuery(a,b) glBeginQueryARB(a,b)
#define glEndQuery(a) glEndQueryARB(a)
#define glGetQueryiv(a,b,c) glGetQueryivARB(a,b,c)
#define glGetQueryObjectiv(a,b,c) glGetQueryObjectivARB(a,b,c)
#define glGetQueryObjectuiv(a,b,c) glGetQueryObjectuivARB(a,b,c)

extern GLuint OccQuerySize;
extern GLint OccResultsAvailable;
extern int OccFailed;
extern int *OccCheckCount;
extern GLuint *OccQueries;
extern void * *OccNodes;
int newOcclude(void);
void zeroOcclusion(void);
extern int QueryCount;
extern GLuint potentialOccluderCount;
extern void* *occluderNodePointer;

#ifdef OCCLUSION
#define OCCLUSIONTEST \
	/* a value of ZERO means that it HAS visible children - helps with initialization */ \
        if ((render_geom!=0) | (render_sensitive!=0)) { \
		/* printf ("OCCLUSIONTEST node %d fl %x\n",node, node->_renderFlags & VF_hasVisibleChildren); */ \
                if ((node->_renderFlags & VF_hasVisibleChildren) == 0) { \
                        /* printf ("WOW - we do NOT need to do this transform but doing it %x!\n",(node->_renderFlags)); \
 printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n", \
         render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */ \
                        return; \
                } \
        } 
#else
#define OCCLUSIONTEST
#endif



#define BEGINOCCLUSIONQUERY \
	if (render_geom) { \
		if (potentialOccluderCount < OccQuerySize) { \
			if (node->__occludeCheckCount < 0) { \
				/* printf ("beginOcclusionQuery, query %u, node %s\n",potentialOccluderCount, stringNodeType(node->_nodeType)); */ \
				glBeginQuery(GL_SAMPLES_PASSED, OccQueries[potentialOccluderCount]); \
				occluderNodePointer[potentialOccluderCount] = (void *)node; \
			} \
		} \
	} 


#define ENDOCCLUSIONQUERY \
	if (render_geom) { \
		if (potentialOccluderCount < OccQuerySize) { \
			if (node->__occludeCheckCount < 0) { \
				/* printf ("glEndQuery node %u\n",node); */ \
				glEndQuery(GL_SAMPLES_PASSED); \
				potentialOccluderCount++; \
			} \
		} \
	} 

#define EXTENTTOBBOX
#define INITIALIZE_EXTENT        node->EXTENT_MAX_X = -10000.0; \
        node->EXTENT_MAX_Y = -10000.0; \
        node->EXTENT_MAX_Z = -10000.0; \
        node->EXTENT_MIN_X = 10000.0; \
        node->EXTENT_MIN_Y = 10000.0; \
        node->EXTENT_MIN_Z = 10000.0;

/********************************
	Verbosity
*********************************/
/* Parsing & Lexing */
#undef CPARSERVERBOSE 

/* Java Class invocation */
#undef JSVRMLCLASSESVERBOSE

/* child node parsing */
#undef CHILDVERBOSE

/* routing */
#undef CRVERBOSE

/* Javascript */
#undef JSVERBOSE

/* sensitive events */
#undef SEVERBOSE

/* Text nodes */
#undef TEXTVERBOSE

/* Texture processing */
#undef TEXVERBOSE

/* streaming from VRML to OpenGL internals. */
#undef STREAM_POLY_VERBOSE

/* collision */
#undef COLLISIONVERBOSE

/* Capabilities of x3dv and x3d */
#undef CAPABILITIESVERBOSE

#ifdef AQUA
#include <CGLTypes.h>
#include <AGL/AGL.h>
#include "aquaInt.h"
extern CGLContextObj myglobalContext;
extern AGLContext aqglobalContext;
#endif

/* number of tesselated coordinates allowed */
#define TESS_MAX_COORDS  500

#define offset_of(p_type,field) ((unsigned int)(&(((p_type)NULL)->field)-NULL))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define UNUSED(v) ((void) v)
#define ISUSED(v) ((void) v)

#define BOOL_STRING(b) (b ? "TRUE" : "FALSE")

#ifdef M_PI
#define PI M_PI
#else
#define PI 3.141592653589793
#endif
/* return TRUE if numbers are very close */
#define APPROX(a,b) (fabs(a-b)<0.00000001)
/* defines for raycasting: */

#define NORMAL_VECTOR_LENGTH_TOLERANCE 0.00001
/* (test if the vector part of a rotation is normalized) */
#define IS_ROTATION_VEC_NOT_NORMAL(rot)        ( \
       fabs(1-sqrt(rot.r[0]*rot.r[0]+rot.r[1]*rot.r[1]+rot.r[2]*rot.r[2])) \
               >NORMAL_VECTOR_LENGTH_TOLERANCE \
)

/* from VRMLC.pm */
extern int displayOpenGLErrors;
extern int sound_from_audioclip;
extern int have_texture;
extern int global_lineProperties;
extern int global_fillProperties;
extern float gl_linewidth;
extern int soundWarned;
extern int cur_hits;
extern struct point_XYZ hyper_r1,hyper_r2;

extern struct X3D_Text *lastTextNode;

/* defines for raycasting: */
#define XEQ (APPROX(t_r1.x,t_r2.x))
#define YEQ (APPROX(t_r1.y,t_r2.y))
#define ZEQ (APPROX(t_r1.z,t_r2.z))
/* xrat(a) = ratio to reach coordinate a on axis x */
#define XRAT(a) (((a)-t_r1.x)/(t_r2.x-t_r1.x))
#define YRAT(a) (((a)-t_r1.y)/(t_r2.y-t_r1.y))
#define ZRAT(a) (((a)-t_r1.z)/(t_r2.z-t_r1.z))
/* mratx(r) = x-coordinate gotten by multiplying by given ratio */
#define MRATX(a) (t_r1.x + (a)*(t_r2.x-t_r1.x))
#define MRATY(a) (t_r1.y + (a)*(t_r2.y-t_r1.y))
#define MRATZ(a) (t_r1.z + (a)*(t_r2.z-t_r1.z))
/* trat: test if a ratio is reasonable */
#undef TRAT
#define TRAT(a) 1
#undef TRAT
#define TRAT(a) ((a) > 0 && ((a) < hpdist || hpdist < 0))

/* POLYREP stuff */
#define POINT_FACES	16 /* give me a point, and it is in up to xx faces */

/* Function Prototypes */

void render_node(struct X3D_Node *node);

void rayhit(float rat, float cx,float cy,float cz, float nx,float ny,float nz,
float tx,float ty, char *descr) ;

void fwnorprint (float *norm);


/* not defined anywhere: */
/* void Extru_init_tex_cap_vals(); */


/* from the PNG examples */
unsigned char  *readpng_get_image(double display_exponent, int *pChannels,
		                       unsigned long *pRowbytes);

/* Used to determine in Group, etc, if a child is a DirectionalLight; do comparison with this */
void DirectionalLight_Rend(void *nod_);
#define DIRLIGHTCHILDREN(a) \
	if ((node->_renderFlags & VF_DirectionalLight)==VF_DirectionalLight)dirlightChildren(a);

#define DIRECTIONAL_LIGHT_OFF if ((node->_renderFlags & VF_DirectionalLight)==VF_DirectionalLight) { \
		lightState(savedlight+1,FALSE); curlight = savedlight; }
#define DIRECTIONAL_LIGHT_SAVE int savedlight = curlight;

void normalize_ifs_face (float *point_normal,
                         struct point_XYZ *facenormals,
                         int *pointfaces,
                        int mypoint,
                        int curpoly,
                        float creaseAngle);


void FW_rendertext(unsigned int numrows,struct Uni_String **ptr,char *directstring, unsigned int nl, double *length,
                double maxext, double spacing, double mysize, unsigned int fsparam,
                struct X3D_PolyRep *rp);


/* Triangulator extern defs - look in CFuncs/Tess.c */
extern struct X3D_PolyRep *global_tess_polyrep;
extern GLUtriangulatorObj *global_tessobj;
extern int global_IFS_Coords[];
extern int global_IFS_Coord_count;

/* do we have to do textures?? */
#define HAVETODOTEXTURES (texture_count != 0)

/* multitexture and single texture handling */
#define MAX_MULTITEXTURE 10

/* texture stuff - see code. Need array because of MultiTextures */
extern GLuint bound_textures[MAX_MULTITEXTURE];
extern int bound_texture_alphas[MAX_MULTITEXTURE];
extern GLint maxTexelUnits;
extern int texture_count; 
extern int     *global_tcin;
extern int     global_tcin_count; 
extern void 	*global_tcin_lastParent;

extern void textureDraw_start(struct X3D_IndexedFaceSet *texC, GLfloat *tex);
extern void textureDraw_end(void);

extern void * this_textureTransform;  /* do we have some kind of textureTransform? */

extern int isTextureLoaded(int texno);
extern int isTextureAlpha(int n);
extern int displayDepth;
extern int display_status;

#define RUNNINGASPLUGIN (isBrowserPlugin)

#define RUNNINGONAMD64 (sizeof(void *) == 8)

/* appearance does material depending on last texture depth */
#define NOTEXTURE 0
#define TEXTURE_NO_ALPHA 1
#define TEXTURE_ALPHA 2

extern int last_texture_type;
extern float global_transparency;

/* what is the max texture size as set by FreeWRL? */
extern GLint global_texSize;


/* Text node system fonts. On startup, freewrl checks to see where the fonts
 * are stored
 */
#define fp_name_len 256
extern char sys_fp[fp_name_len];


extern float AC_LastDuration[];

extern int SoundEngineStarted;

/* Material optimizations */
void do_shininess (GLenum face, float shininess);
void do_glMaterialfv (GLenum face, GLenum pname, GLfloat *param);

/* used to determine whether we have transparent materials. */
extern int have_transparency;


/* current time */
extern double TickTime;
extern double lastTime;

/* number of triangles this rendering loop */
extern int trisThisLoop;


/* Transform node optimizations */
int verify_rotate(GLfloat *params);
int verify_translate(GLfloat *params);
int verify_scale(GLfloat *params);

void mark_event (struct X3D_Node *from, unsigned int fromoffset);
void mark_event_check (struct X3D_Node *from, unsigned int fromoffset,char *fn, int line);

/* saved rayhit and hyperhit */
extern struct SFColor ray_save_posn, hyp_save_posn, hyp_save_norm;

/* set a node to be sensitive */
void setSensitive(struct X3D_Node *parent,struct X3D_Node *me);

/* bindable nodes */
extern GLint viewport[];
extern GLdouble fieldofview;
extern struct point_XYZ ViewerUpvector;
extern struct sNaviInfo naviinfo;
extern double defaultExamineDist;


/* Sending events back to Browser (eg, Anchor) */
extern int BrowserAction;
extern struct X3D_Anchor *AnchorsAnchor;
int checkIfX3DVRMLFile(char *fn);
void EAI_Anchor_Response (int resp);
extern int wantEAI;
struct Uni_String *newASCIIString(char *str);
void verify_Uni_String(struct  Uni_String *unis, char *str);

void *returnInterpolatorPointer (const char *x);

/* SAI code node interface return values  The meanings of
   these numbers can be found in the SAI java code */
#define X3DBoundedObject			1
#define X3DBounded2DObject 		 	2
#define X3DURLObject 			 	3
#define X3DAppearanceNode 			10
#define X3DAppearanceChildNode 			11
#define X3DMaterialNode 			12
#define X3DTextureNode 				13
#define X3DTexture2DNode 			14
#define X3DTexture3DNode 			15
#define X3DTextureTransformNode  		16
#define X3DTextureTransform2DNode 		17
#define X3DGeometryNode 			18
#define X3DTextNode 				19
#define X3DParametricGeometryNode 		20
#define X3DGeometricPropertyNode 		21
#define X3DColorNode				22
#define X3DCoordinateNode 			23
#define X3DNormalNode 				24
#define X3DTextureCoordinateNode 		25
#define X3DFontStyleNode 			26
#define X3DProtoInstance 			27
#define X3DChildNode 				28
#define X3DBindableNode 			29
#define X3DBackgroundNode 			30
#define X3DGroupingNode 			31
#define X3DShapeNode 				32
#define X3DInterpolatorNode 			33
#define X3DLightNode 				34
#define X3DScriptNode 				35
#define X3DSensorNode 				36
#define X3DEnvironmentalSensorNode 		37
#define X3DKeyDeviceSensorNode 			38
#define X3DNetworkSensorNode 			39
#define X3DPointingDeviceSensorNode 		40
#define X3DDragSensorNode 			41
#define X3DTouchSensorNode 			42
#define X3DSequencerNode  			43
#define X3DTimeDependentNode 			44
#define X3DSoundSourceNode 			45
#define X3DTriggerNode 				46
#define X3DInfoNode 				47
#define X3DShaderNode				48
#define X3DVertexAttributeNode			49
#define X3DProgrammableShaderObject		50
#define X3DUrlObject				51
#define X3DEnvironmentTextureNode 52




void CRoutes_js_new (uintptr_t num,int scriptType);
extern int max_script_found;
extern int max_script_found_and_initialized;
void getMFNodetype (char *strp, struct Multi_Node *ch, struct X3D_Node *par, int ar);
void AddRemoveChildren (struct X3D_Node *parent, struct Multi_Node *tn, uintptr_t *nodelist, int len, int ar);

void update_node(struct X3D_Node *ptr);
void update_renderFlag(struct X3D_Node *ptr, int flag);

int JSparamIndex (char *name, char *type);

/* setting script eventIns from routing table or EAI */
void Set_one_MultiElementtype (uintptr_t tn, uintptr_t tptr, void *fn, unsigned len);
void set_one_ECMAtype (uintptr_t tonode, int toname, int dataType, void *Data, unsigned datalen);
void mark_script (uintptr_t num);


/* structure for rayhits */
struct currayhit {
	struct X3D_Node *node; /* What node hit at that distance? */
	GLdouble modelMatrix[16]; /* What the matrices were at that node */
	GLdouble projMatrix[16];
};



void JSMaxAlloc(void);
void cleanupDie(uintptr_t num, const char *msg);
void shutdown_EAI(void);
uintptr_t EAI_GetNode(const char *str);
unsigned int EAI_GetViewpoint(const char *str);
void EAI_killBindables (void);
void EAI_GetType (uintptr_t cNode,  char *ctmp, char *dtmp, uintptr_t *cNodePtr, uintptr_t *fieldOffset,
                        uintptr_t *dataLen, uintptr_t *typeString,  unsigned int *scripttype, int *accessType);


void setScriptECMAtype(uintptr_t);
void resetScriptTouchedFlag(int actualscript, int fptr);
int get_touched_flag(uintptr_t fptr, uintptr_t actualscript);
void getMultiElementtype(char *strp, struct Multi_Vec3f *tn, int eletype);
void setScriptMultiElementtype(uintptr_t);
void Parser_scanStringValueToMem(struct X3D_Node *ptr, int coffset, int ctype, char *value);
void Multimemcpy(void *tn, void *fn, int len);
void CRoutes_RegisterSimple(struct X3D_Node* from, int fromOfs,
 struct X3D_Node* to, int toOfs, int len, int dir);
void CRoutes_Register(int adrem,        struct X3D_Node *from,
                                 int fromoffset,
                                 unsigned int to_count,
                                 char *tonode_str,
                                 int length,
                                 void *intptr,
                                 int scrdir,
                                 int extra);
void CRoutes_free(void);
void propagate_events(void);
int getRoutesCount(void);
void getSpecificRoute (int routeNo, uintptr_t *fromNode, int *fromOffset, 
                uintptr_t *toNode, int *toOffset);
void sendScriptEventIn(uintptr_t num);
void getField_ToJavascript (int num, int fromoffset);
void add_first(struct X3D_Node * node);
void registerTexture(struct X3D_Node * node);
void registerMIDINode(struct X3D_Node *node);
int checkNode(struct X3D_Node *node, char *fn, int line);


void do_first(void);
void process_eventsProcessed(void);

void getEAI_MFStringtype (struct Multi_String *from, struct Multi_String *to);


extern struct CRscriptStruct *ScriptControl; /* Script invocation parameters */
extern uintptr_t *scr_act;    /* script active array - defined in CRoutes.c */
extern int *thisScriptType;    /* what kind of script this is - in CRoutes.c */
extern int JSMaxScript;  /* defined in JSscipts.c; maximum size of script arrays */
void JSCreateScriptContext(uintptr_t num); 
void JSInitializeScriptAndFields (uintptr_t num);
void SaveScriptText(uintptr_t num, char *text);


void update_status(char* msg);
void kill_status();

/* menubar stuff */
void frontendUpdateButtons(void); /* used only if we are not able to multi-thread OpenGL */
void setMenuButton_collision (int val) ;
void setMenuButton_headlight (int val) ;
void setMenuButton_navModes (int type) ;
void setConsoleMessage(char *stat) ;
void setMenuStatus(char *stat) ;
void setMenuFps (float fps) ;
void setMenuButton_texSize (int size);
extern int textures_take_priority;
void setTextures_take_priority (int x);
extern int useExperimentalParser;
void setUseCParser (int x);
extern int useShapeThreadIfPossible;
void setUseShapeThreadIfPossible(int x);

int convert_typetoInt (const char *type);	/* convert a string, eg "SFBOOL" to type, eg SFBOOL */

extern double BrowserFPS;
extern double BrowserSpeed;
void render_polyrep(void *node);

extern int CRoutesExtra;		/* let EAI see param of routing table - Listener data. */

/* types of scripts. */
#define NOSCRIPT 	0
#define JAVASCRIPT	1
#define SHADERSCRIPT	4

/* printf is defined by perl; causes segfault in threaded freewrl */
#ifdef printf
#undef printf
#endif
#ifdef die
#undef die
#endif


/* types to tell the Perl thread what to handle */
#define FROMSTRING 	1
#define	FROMURL		2
#define INLINE		3
#define ZEROBINDABLES   8   /* get rid of Perl datastructures */
#define FROMCREATENODE	13  /* create a node by just giving its node type */
#define FROMCREATEPROTO	14  /* create a node by just giving its node type */
#define UPDATEPROTOD	16  /* update a PROTO definition */
#define GETPROTOD	17  /* update a PROTO definition */



extern void *rootNode;
extern int isPerlParsing(void);
extern int isURLLoaded(void);	/* initial scene loaded? Robert Sim */
extern int isTextureParsing(void);
extern void loadInline(struct X3D_Inline *node);
extern void loadTextureNode(struct X3D_Node *node,  void *param);
extern void loadMovieTexture(struct X3D_MovieTexture *node,  void *param);
extern void loadMultiTexture(struct X3D_MultiTexture *node);
extern void loadBackgroundTextures (struct X3D_Background *node);
extern void loadTextureBackgroundTextures (struct X3D_TextureBackground *node);
extern GLfloat boxtex[], boxnorms[], BackgroundVert[];
extern GLfloat Backtex[], Backnorms[];

extern void new_tessellation(void);
extern void initializePerlThread(void);
extern void setGeometry (const char *optarg);
extern void setWantEAI(int flag);
extern void setPluginPipe(const char *optarg);
extern void setPluginFD(const char *optarg);
extern void setPluginInstance(const char *optarg);

/* shutter glasses, stereo view  from Mufti@rus */
extern void setShutter (void);
#ifndef AQUA
extern int shutterGlasses;
#endif
extern void setScreenDist (const char *optArg);
extern void setStereoParameter (const char *optArg);
extern void setEyeDist (const char *optArg);

extern int isPerlinitialized(void);

#ifdef HAVE_MOTIF
	#define ISDISPLAYINITIALIZED isMotifDisplayInitialized()
	#define GET_GLWIN getMotifWindowedGLwin (&GLwin);
	#define OPEN_TOOLKIT_MAINWINDOW openMotifMainWindow (argc, argv);
	#define CREATE_TOOLKIT_MAIN_WINDOW createMotifMainWindow();
	int isMotifDisplayInitialized(void);
	void getMotifWindowedGLwin (Window *);
	void openMotifMainWindow (int argc, char ** argv);
	void createMotifMainWindow(void);
#else

	#ifdef HAVE_GTK2
		#define ISDISPLAYINITIALIZED isGtkDisplayInitialized()
		#define GET_GLWIN getGtkWindowedGLwin (&GLwin);
		#define OPEN_TOOLKIT_MAINWINDOW openGtkMainWindow (argc, argv);
		#define CREATE_TOOLKIT_MAIN_WINDOW createGtkMainWindow();
		int isGtkDisplayInitialized(void);
		void getGtkWindowedGLwin (Window *);
		void openGtkMainWindow (int argc, char ** argv);
		void createGtkMainWindow(void);

	#else
		#define HAVE_NOTOOLKIT
		#define ISDISPLAYINITIALIZED TRUE
		#define GET_GLWIN getBareWindowedGLwin (&GLwin);
		#define OPEN_TOOLKIT_MAINWINDOW openBareMainWindow (argc, argv);
		#define CREATE_TOOLKIT_MAIN_WINDOW createBareMainWindow();
	#endif
#endif

extern char *getInputURL(void);
extern char *lastReadFile; 		/* name last file read in */
extern int  lightingOn;			/* state of GL_LIGHTING */
extern int cullFace;			/* state of GL_CULL_FACE */
extern double hpdist;			/* in VRMLC.pm */
extern struct point_XYZ hp;			/* in VRMLC.pm */
extern void *hypersensitive; 		/* in VRMLC.pm */
extern int hyperhit;			/* in VRMLC.pm */
extern struct point_XYZ r1, r2;		/* in VRMLC.pm */
extern struct sCollisionInfo CollisionInfo;
extern struct currayhit rayHit,rayph,rayHitHyper;
extern GLint smooth_normals;

extern void xs_init(void);

extern int navi_tos;
extern void initializeTextureThread(void);
extern int isTextureinitialized(void);
extern int fileExists(char *fname, char *firstBytes, int GetIt, int *isTemp);
extern void checkAndAllocMemTables(int *texture_num, int increment);
extern void   storeMPGFrameData(int latest_texture_number, int h_size, int v_size,
        int mt_repeatS, int mt_repeatT, char *Image);
void mpg_main(char *filename, int *x,int *y,int *depth,int *frameCount,void **ptr);
int getValidFileFromUrl (char *filename, char *path, struct Multi_String *inurl, char *firstBytes, int* removeIt);
void removeFilenameFromPath (char *path);

int EAI_CreateVrml(const char *tp, const char *inputstring, uintptr_t *retarr, int retarrsize);
void EAI_Route(char cmnd, const char *tf);
void EAI_replaceWorld(const char *inputstring);

void render_hier(struct X3D_Node *p, int rwhat);
void handle_EAI(void);
void handle_aqua(const int mev, const unsigned int button, int x, int y);


extern int screenWidth, screenHeight;


#define overMark        23425
/* mimic X11 events in AQUA */
#ifdef AQUA
#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6
#define MapNotify       19

#endif

/* Unix front end height/width */
#ifndef AQUA
extern int feWidth, feHeight;
#endif

/* SD AQUA FUNCTIONS */
#ifdef AQUA
extern int getOffset();
extern void initGL();
extern void setButDown(int button, int value);
extern void setCurXY(int x, int y);
extern void do_keyPress (char ch, int ev);
extern void setLastMouseEvent(int etype);
extern void initFreewrl();
extern void aqDisplayThread();
#endif
extern void setSnapSeq();
extern void setEAIport(int pnum);
extern void setTexSize(int num);
extern void setKeyString(const char *str);
extern void setNoCollision();
extern void setSnapGif();
extern void setLineWidth(float lwidth);
extern void closeFreewrl();
extern void setSeqFile(const char* file);
extern void setSnapFile(const char* file);
extern void setMaxImages(int max);
extern void setBrowserFullPath(const char *str);
extern void setSeqTemp(const char* file);
extern void setInstance(uintptr_t instance);
extern void setScreenDim(int w, int h);

extern const char *getLibVersion();
extern void doBrowserAction ();


extern char *myPerlInstallDir;

/* for Extents and BoundingBoxen */
#define EXTENT_MAX_X _extent[0]
#define EXTENT_MIN_X _extent[1]
#define EXTENT_MAX_Y _extent[2]
#define EXTENT_MIN_Y _extent[3]
#define EXTENT_MAX_Z _extent[4]
#define EXTENT_MIN_Z _extent[5]
void setExtent (float maxx, float minx, float maxy, float miny, float maxz, float minz, struct X3D_Node *this_);

#define RECORD_DISTANCE if (render_geom) recordDistance (X3D_NODE(node));
void recordDistance(struct X3D_Node *nod);

void propagateExtent (struct X3D_Node *this_);

#ifdef DISPLAYBOUNDINGBOX
void BoundingBox(struct X3D_Node* node);
#define BOUNDINGBOX if (render_geom && (!render_blend)) BoundingBox (X3D_NODE(node));
#else
#define BOUNDINGBOX
#endif

void freewrlDie(const char *format);

extern double nearPlane, farPlane, screenRatio, calculatedNearPlane, calculatedFarPlane;

/* children stuff moved out of VRMLRend.pm and VRMLC.pm for v1.08 */

extern int render_sensitive,render_vp,render_light,render_proximity,curlight,verbose,render_blend,render_geom,render_collision;

extern void XEventStereo();


/* Java CLASS invocation */
int newJavaClass(int scriptInvocationNumber,char * nodestr,char *node);
int initJavaClass(int scriptno);

int SAI_IntRetCommand (char cmnd, const char *fn);
char * SAI_StrRetCommand (char cmnd, const char *fn);
char *EAI_GetTypeName (unsigned int uretval);
char* EAI_GetValue(unsigned int nodenum, const char *fieldname, const char *nodename);
void setCLASStype (uintptr_t num);
void sendCLASSEvent(uintptr_t fn, int scriptno, char *fieldName, int type, int len);
void processClassEvents(int scriptno, int startEntry, int endEntry);
char *processThisClassEvent (void *fn, int startEntry, int endEntry, char *buf);
void getCLASSMultNumType (char *buf, int bufSize,
	struct Multi_Vec3f *tn,
	struct X3D_Node *parent,
	int eletype, int addChild);

void fwGetDoublev (int ty, double *mat);
void fwMatrixMode (int mode);
void fwXformPush(void);
void fwXformPop(void);
void fwLoadIdentity (void);
void invalidateCurMat(void);
void doBrowserAction (void);
void add_parent(struct X3D_Node *node_, struct X3D_Node *parent_,char *file, int line);
void remove_parent(struct X3D_Node *child, struct X3D_Node *parent);
void EAI_readNewWorld(char *inputstring);
void make_indexedfaceset(struct X3D_IndexedFaceSet *this_);

void render_LoadSensor(struct X3D_LoadSensor *this);

void render_Text (struct X3D_Text * this_);
#define rendray_Text render_ray_polyrep
void make_Text (struct X3D_Text * this_);
void collide_Text (struct X3D_Text * this_);
void render_TextureCoordinateGenerator(struct X3D_TextureCoordinateGenerator *this);
void render_TextureCoordinate(struct X3D_TextureCoordinate *this);

/* Component Grouping */
void prep_Transform (struct X3D_Transform *this_);
void fin_Transform (struct X3D_Transform *this_);
void child_Transform (struct X3D_Transform *this_);
void prep_Group (struct X3D_Group *this_);
void child_Group (struct X3D_Group *this_);
void child_StaticGroup (struct X3D_StaticGroup *this_);
void child_Switch (struct X3D_Switch *this_);

void changed_Group (struct X3D_Group *this_);
void changed_Switch (struct X3D_Switch *this_);
void changed_StaticGroup (struct X3D_StaticGroup *this_);
void changed_Transform (struct X3D_Transform *this_);

/* Environmental Sensor nodes */
void proximity_ProximitySensor (struct X3D_ProximitySensor *this_);
void child_VisibilitySensor (struct X3D_VisibilitySensor *this_);

/* Navigation Component */
void prep_Billboard (struct X3D_Billboard *this_);
void proximity_Billboard (struct X3D_Billboard *this_);
void changed_Billboard (struct X3D_Billboard *this_);
void prep_Viewpoint(struct X3D_Viewpoint *node);
void child_Billboard (struct X3D_Billboard *this_);
void child_LOD (struct X3D_LOD *this_);
void proximity_LOD (struct X3D_LOD *this_);
void fin_Billboard (struct X3D_Billboard *this_);
void child_Collision (struct X3D_Collision *this_);
void changed_Collision (struct X3D_Collision *this_);


/* HAnim Component */
void prep_HAnimJoint (struct X3D_HAnimJoint *this);
void prep_HAnimSite (struct X3D_HAnimSite *this);

void child_HAnimHumanoid(struct X3D_HAnimHumanoid *this_); 
void child_HAnimJoint(struct X3D_HAnimJoint *this_); 
void child_HAnimSegment(struct X3D_HAnimSegment *this_); 
void child_HAnimSite(struct X3D_HAnimSite *this_); 

void render_HAnimHumanoid (struct X3D_HAnimHumanoid *node);
void render_HAnimJoint (struct X3D_HAnimJoint * node);

void fin_HAnimSite (struct X3D_HAnimSite *this_);
void fin_HAnimJoint (struct X3D_HAnimJoint *this_);

void changed_HAnimSite (struct X3D_HAnimSite *this_);



/* Sound Component */
void render_Sound (struct X3D_Sound *this_);
void render_AudioControl (struct X3D_AudioControl *this_);
void render_AudioClip (struct X3D_AudioClip *this_);

/* Texturing Component */
void render_PixelTexture (struct X3D_PixelTexture *this_);
void render_ImageTexture (struct X3D_ImageTexture *this_);
void render_MultiTexture (struct X3D_MultiTexture *this_);
void render_MovieTexture (struct X3D_MovieTexture *this_);

/* Shape Component */
void render_Appearance (struct X3D_Appearance *this_);
void render_FillProperties (struct X3D_FillProperties *this_);
void render_LineProperties (struct X3D_LineProperties *this_);
void render_Material (struct X3D_Material *this_);
void render_TwoSidedMaterial (struct X3D_TwoSidedMaterial *this_);
void render_Shape (struct X3D_Shape *this_);
void child_Shape (struct X3D_Shape *this_);
void child_Appearance (struct X3D_Appearance *this_);

/* Geometry3D nodes */
void render_Box (struct X3D_Box *this);
void compile_Box (struct X3D_Box *this);
void collide_Box (struct X3D_Box *this);
void render_Cone (struct X3D_Cone *this);
void compile_Cone (struct X3D_Cone *this);
void collide_Cone (struct X3D_Cone *this);
void render_Cylinder (struct X3D_Cylinder *this);
void compile_Cylinder (struct X3D_Cylinder *this);
void collide_Cylinder (struct X3D_Cylinder *this);
void render_ElevationGrid (struct X3D_ElevationGrid *this);
#define rendray_ElevationGrid  render_ray_polyrep
#define collide_ElevationGrid collide_IndexedFaceSet
void render_Extrusion (struct X3D_Extrusion *this);
void collide_Extrusion (struct X3D_Extrusion *this);
#define rendray_Extrusion render_ray_polyrep
void render_IndexedFaceSet (struct X3D_IndexedFaceSet *this);
void collide_IndexedFaceSet (struct X3D_IndexedFaceSet *this);
#define rendray_IndexedFaceSet render_ray_polyrep 
void render_IndexedFaceSet (struct X3D_IndexedFaceSet *this);
void render_Sphere (struct X3D_Sphere *this);
void compile_Sphere (struct X3D_Sphere *this);
void collide_Sphere (struct X3D_Sphere *this);
void make_Extrusion (struct X3D_Extrusion *this);
#define make_IndexedFaceSet make_indexedfaceset
#define make_ElevationGrid make_indexedfaceset
#define rendray_ElevationGrid render_ray_polyrep
void rendray_Box (struct X3D_Box *this_);
void rendray_Sphere (struct X3D_Sphere *this_);
void rendray_Cylinder (struct X3D_Cylinder *this_);
void rendray_Cone (struct X3D_Cone *this_);

/* Geometry2D nodes */
void render_Arc2D (struct X3D_Arc2D *this_);
void compile_Arc2D (struct X3D_Arc2D *this_);
void render_ArcClose2D (struct X3D_ArcClose2D *this_);
void compile_ArcClose2D (struct X3D_ArcClose2D *this_);
void render_Circle2D (struct X3D_Circle2D *this_);
void compile_Circle2D (struct X3D_Circle2D *this_);
void render_Disk2D (struct X3D_Disk2D *this_);
void compile_Disk2D (struct X3D_Disk2D *this_);
void render_Polyline2D (struct X3D_Polyline2D *this_);
void render_Polypoint2D (struct X3D_Polypoint2D *this_);
void render_Rectangle2D (struct X3D_Rectangle2D *this_);
void compile_Rectangle2D (struct X3D_Rectangle2D *this_);
void render_TriangleSet2D (struct X3D_TriangleSet2D *this_);
void compile_TriangleSet2D (struct X3D_TriangleSet2D *this_);
void collide_Disk2D (struct X3D_Disk2D *this_);
void collide_Rectangle2D (struct X3D_Rectangle2D *this_);
void collide_TriangleSet2D (struct X3D_TriangleSet2D *this_);

/* Component Rendering nodes */
#define rendray_IndexedTriangleSet render_ray_polyrep
#define rendray_IndexedTriangleFanSet render_ray_polyrep
#define rendray_IndexedTriangleStripSet render_ray_polyrep
#define rendray_TriangleSet render_ray_polyrep
#define rendray_TriangleFanSet render_ray_polyrep
#define rendray_TriangleStripSet render_ray_polyrep
void render_IndexedTriangleFanSet (struct X3D_IndexedTriangleFanSet *this_); 
void render_IndexedTriangleSet (struct X3D_IndexedTriangleSet *this_); 
void render_IndexedTriangleStripSet (struct X3D_IndexedTriangleStripSet *this_); 
void render_TriangleFanSet (struct X3D_TriangleFanSet *this_); 
void render_TriangleStripSet (struct X3D_TriangleStripSet *this_); 
void render_TriangleSet (struct X3D_TriangleSet *this_); 
void render_LineSet (struct X3D_LineSet *this_); 
void render_IndexedLineSet (struct X3D_IndexedLineSet *this_); 
void render_PointSet (struct X3D_PointSet *this_); 
#define collide_IndexedTriangleFanSet  collide_IndexedFaceSet
#define collide_IndexedTriangleSet  collide_IndexedFaceSet
#define collide_IndexedTriangleStripSet  collide_IndexedFaceSet
#define collide_TriangleFanSet  collide_IndexedFaceSet
#define collide_TriangleSet  collide_IndexedFaceSet
#define collide_TriangleStripSet  collide_IndexedFaceSet
#define make_IndexedTriangleFanSet  make_indexedfaceset
#define make_IndexedTriangleSet  make_indexedfaceset
#define make_IndexedTriangleStripSet  make_indexedfaceset
#define make_TriangleFanSet  make_indexedfaceset
#define make_TriangleSet  make_indexedfaceset
#define make_TriangleStripSet  make_indexedfaceset
void compile_LineSet (struct X3D_LineSet *this_); 
void compile_IndexedLineSet (struct X3D_IndexedLineSet *this_); 

/* Component Lighting Nodes */
void render_DirectionalLight (struct X3D_DirectionalLight *this_);
void prep_SpotLight (struct X3D_SpotLight *this_);
void prep_PointLight (struct X3D_PointLight *this_);

/* Geospatial nodes */
int checkX3DGeoElevationGridFields (struct X3D_ElevationGrid *node, float **points, int *npoints);
void render_GeoElevationGrid (struct X3D_GeoElevationGrid *this_);
void rendray_GeoElevationGrid (struct X3D_GeoElevationGrid *this_);
void collide_GeoElevationGrid (struct X3D_GeoElevationGrid *this_);
void make_GeoElevationGrid (struct X3D_GeoElevationGrid *this_);
void prep_GeoLocation (struct X3D_GeoLocation *this_);
void prep_GeoViewpoint (struct X3D_GeoViewpoint *this_);
void fin_GeoLocation (struct X3D_GeoLocation *this_);
void changed_GeoLocation (struct X3D_GeoLocation *this_);
void child_GeoLOD (struct X3D_GeoLOD *this_);
void proximity_GeoLOD (struct X3D_GeoLOD *this_);
void child_GeoLocation (struct X3D_GeoLocation *this_);
void compile_GeoCoordinate (struct X3D_GeoCoordinate * this);
void compile_GeoElevationGrid (struct X3D_GeoElevationGrid * this);
void compile_GeoLocation (struct X3D_GeoLocation * this);
void compile_GeoLOD (struct X3D_GeoLOD * this);
void compile_GeoMetadata (struct X3D_GeoMetadata * this);
void compile_GeoOrigin (struct X3D_GeoOrigin * this);
void compile_GeoPositionInterpolator (struct X3D_GeoPositionInterpolator * this);
void compile_GeoTouchSensor (struct X3D_GeoTouchSensor * this);
void compile_GeoViewpoint (struct X3D_GeoViewpoint * this);
void compile_GeoProximitySensor (struct X3D_GeoProximitySensor *this);
void compile_GeoTransform (struct X3D_GeoTransform * node);
void proximity_GeoProximitySensor (struct X3D_GeoProximitySensor *this);
void prep_GeoTransform (struct X3D_GeoTransform *);
void child_GeoTransform (struct X3D_GeoTransform *);
void fin_GeoTransform (struct X3D_GeoTransform *);
void changed_GeoTransform (struct X3D_GeoTransform *);


/* Networking Component */
void child_Anchor (struct X3D_Anchor *this_);
void child_Inline (struct X3D_Inline *this_);
void changed_Inline (struct X3D_Inline *this_);
void changed_Anchor (struct X3D_Anchor *this_);

/* KeyDevice Component */
void killKeySensorNodeList(void);
void addNodeToKeySensorList(struct X3D_Node* node);
int KeySensorNodePresent(void);
void sendKeyToKeySensor(const char key, int upDown);

/* Programmable Shaders Component */
void render_ComposedShader (struct X3D_ComposedShader *);
void render_PackagedShader (struct X3D_PackagedShader *);
void render_ProgramShader (struct X3D_ProgramShader *);
void compile_ComposedShader (struct X3D_ComposedShader *);
void compile_PackagedShader (struct X3D_PackagedShader *);
void compile_ProgramShader (struct X3D_ProgramShader *);
#ifdef GL_VERSION_2_0
	#define TURN_APPEARANCE_SHADER_OFF \
		{extern GLuint globalCurrentShader; if (globalCurrentShader!=0) { globalCurrentShader = 0; glUseProgram(0);}}
#else
	#ifdef GL_VERSION_1_5
		#define TURN_APPEARANCE_SHADER_OFF \
			{extern GLuint globalCurrentShader; if (globalCurrentShader!=0) { globalCurrentShader = 0; glUseProgramObjectARB(0);}}
	#else
		#define TURN_APPEARANCE_SHADER_OFF
	#endif
#endif

void prep_MidiControl (struct X3D_MidiControl *node);
void do_MidiControl (void *node);
void MIDIRegisterMIDI(char *str);
/* ReWire device/controller  table */
struct ReWireDeviceStruct {
        struct X3D_MidiControl* node;   /* pointer to the node that controls this */
        int encodedDeviceName;          /* index into ReWireNamenames */
        int bus;                        /* which MIDI bus this is */
        int channel;                    /* which MIDI channel on this bus it is */
        int encodedControllerName;      /* index into ReWireNamenames */
        int controller;                 /* controller number */
        int cmin;                       /* minimum value for this controller */
        int cmax;                       /* maximum value for this controller */
        int ctype;                      /* controller type TYPE OF FADER control - not used currently */
};

/* ReWireName table */
struct ReWireNamenameStruct {
        char *name;
};

extern struct ReWireNamenameStruct *ReWireNamenames;
extern int ReWireNametableSize;
extern int MAXReWireNameNames;
extern struct ReWireDeviceStruct *ReWireDevices;
extern int ReWireDevicetableSize;
extern int MAXReWireDevices;


/* Event Utilities Component */
void do_BooleanFilter (void *node);
void do_BooleanSequencer (void *node);
void do_BooleanToggle (void *node);
void do_BooleanTrigger (void *node);
void do_IntegerSequencer (void *node);
void do_IntegerTrigger (void *node);
void do_TimeTrigger (void *node);


#define ADD_PARENT(a,b) add_parent(a,b,__FILE__,__LINE__)
#define NODE_ADD_PARENT(a) ADD_PARENT(a,X3D_NODE(ptr))
#define NODE_REMOVE_PARENT(a) ADD_PARENT(a,X3D_NODE(ptr))


/* OpenGL state cache */
/* solid shapes, GL_CULL_FACE can be enabled if a shape is Solid */ 
#define CULL_FACE(v) /* printf ("nodeSolid %d cullFace %d GL_FALSE %d FALSE %d\n",v,cullFace,GL_FALSE,FALSE); */ \
		if (v != cullFace) {	\
			cullFace = v; \
			if (cullFace == 1) glEnable(GL_CULL_FACE);\
			else glDisable(GL_CULL_FACE);\
		}
#define DISABLE_CULL_FACE CULL_FACE(0)
#define ENABLE_CULL_FACE CULL_FACE(1)
#define CULL_FACE_INITIALIZE cullFace=0; glDisable(GL_CULL_FACE);

#define LIGHTING_ON if (!lightingOn) {lightingOn=TRUE;glEnable(GL_LIGHTING);}
#define LIGHTING_OFF if(lightingOn) {lightingOn=FALSE;glDisable(GL_LIGHTING);}
#define LIGHTING_INITIALIZE lightingOn=TRUE; glEnable(GL_LIGHTING);

void zeroAllBindables(void);
void Next_ViewPoint(void);
void Prev_ViewPoint(void);
void First_ViewPoint(void);
void Last_ViewPoint(void);

int freewrlSystem (const char *string);

int inputParse(unsigned type, char *inp, int bind, int returnifbusy,
                        void *ptr, unsigned ofs, int *complete,
                        int zeroBind);
void compileNode (void (*nodefn)(void *, void *, void *, void *, void *), void *node, void *a, void *b, void *c, void *d);
void destroyCParserData();

void getMovieTextureOpenGLFrames(int *highest, int *lowest,int myIndex);


int ConsoleMessage(const char *fmt, ...);

void closeConsoleMessage(void);
extern int consMsgCount;

void outOfMemory(const char *message);

void killErrantChildren(void);

void kill_routing(void);
void kill_bindables(void);
void kill_javascript(void);
void kill_oldWorld(int a, int b, int c);
void kill_clockEvents(void);
void kill_openGLTextures(void);
void kill_X3DDefs(void);
extern int currentFileVersion;

int findFieldInFIELDNAMES(const char *field);
int findFieldInFIELD(const char* field);
int findFieldInFIELDTYPES(const char *field);
int findFieldInX3DACCESSORS(const char *field);
int findFieldInEXPOSED_FIELD(const char* field);
int findFieldInEVENT_IN(const char* field);
int findFieldInEVENT_OUT(const char* field);
int findFieldInX3DSPECIALKEYWORDS(const char *field);
int findFieldInGEOSPATIAL(const char *field);

/* Values for fromTo */
#define ROUTED_FIELD_EVENT_OUT 0
#define ROUTED_FIELD_EVENT_IN  1
int findRoutedFieldInFIELDNAMES(struct X3D_Node *node, const char *field, int fromTo);
int findRoutedFieldInEXPOSED_FIELD(struct X3D_Node*, const char*, int);
int findRoutedFieldInEVENT_IN(struct X3D_Node*, const char*, int);
int findRoutedFieldInEVENT_OUT(struct X3D_Node*, const char*, int);
int findFieldInNODES(const char *node);
int findFieldInCOMPONENTS(const char *node);
int findFieldInPROFILESS(const char *node);
int findFieldInALLFIELDNAMES(const char *field);
void findFieldInOFFSETS(const int *nodeOffsetPtr, const int field, int *coffset, int *ctype, int *ckind);
char *findFIELDNAMESfromNodeOffset(struct X3D_Node *node, int offset);
int findFieldInKEYWORDS(const char *field);
int findFieldInPROTOKEYWORDS(const char *field);
int countCommas (char *instr);
void sortChildren (struct Multi_Node ch);
void dirlightChildren(struct Multi_Node ch);
void normalChildren(struct Multi_Node ch);
void checkParentLink (struct X3D_Node *node,struct X3D_Node *parent);

/* background colour */
void setglClearColor (float *val); 
void doglClearColor(void);
extern int cc_changed;
extern int forceBackgroundRecompile;

int mapFieldTypeToInernaltype (indexT kwIndex);
void finishEventLoop();
void resetEventLoop();

/* MIDI stuff... */
void ReWireRegisterMIDI (char *str);
void ReWireMIDIControl(char *str);


/* META data, component, profile  stuff */
void handleMetaDataStringString(struct Uni_String *val1,struct Uni_String *val2);
void handleProfile(int myp);
void handleComponent(int com, int lev);
void handleExport (char *node, char *as);
void handleImport (char *nodeName,char *nodeImport, char *as);
void handleVersion (float lev);


/* free memory */
void registerX3DNode(struct X3D_Node * node);

void doNotRegisterThisNodeForDestroy(struct X3D_Node * nodePtr);

struct Multi_Vec3f *getCoordinate (void *node, char *str);

#ifdef AQUA
Boolean notFinished();
void disposeContext();
void setPaneClipRect(int npx, int npy, WindowPtr fwWindow, int ct, int cb, int cr, int cl, int width, int height);
void createContext(CGrafPtr grafPtr);
void setIsPlugin();
void sendPluginFD(int fd);
void aquaPrintVersion();
void setPluginPath(char* path);
#endif
void setEaiVerbose();
void replaceWorldNeeded(char* str);

/* X3D C parser */
int X3DParse (struct X3D_Group *parent, char *inputstring);
void *createNewX3DNode (int nt);

/* threading stuff */
extern pthread_t PCthread;
extern pthread_t shapeThread;
extern pthread_t loadThread;

/* node binding */
extern void *setViewpointBindInRender;
extern void *setFogBindInRender;
extern void *setBackgroundBindInRender;
extern void *setNavigationBindInRender;


/* ProximitySensor and GeoProximitySensor are same "code" at this stage of the game */
#define PROXIMITYSENSOR(type,center,initializer1,initializer2) \
void proximity_##type (struct X3D_##type *node) { \
	/* Viewer pos = t_r2 */ \
	double cx,cy,cz; \
	double len; \
	struct point_XYZ dr1r2; \
	struct point_XYZ dr2r3; \
	struct point_XYZ nor1,nor2; \
	struct point_XYZ ins; \
	static const struct point_XYZ yvec = {0,0.05,0}; \
	static const struct point_XYZ zvec = {0,0,-0.05}; \
	static const struct point_XYZ zpvec = {0,0,0.05}; \
	static const struct point_XYZ orig = {0,0,0}; \
	struct point_XYZ t_zvec, t_yvec, t_orig; \
	GLdouble modelMatrix[16]; \
	GLdouble projMatrix[16]; \
 \
	if(!((node->enabled))) return; \
	initializer1 \
	initializer2 \
 \
	/* printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/ \
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/ \
 \
	/* transforms viewers coordinate space into sensors coordinate space. \
	 * this gives the orientation of the viewer relative to the sensor. \
	 */ \
	fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix); \
	fwGetDoublev(GL_PROJECTION_MATRIX, projMatrix); \
	gluUnProject(orig.x,orig.y,orig.z,modelMatrix,projMatrix,viewport, \
		&t_orig.x,&t_orig.y,&t_orig.z); \
	gluUnProject(zvec.x,zvec.y,zvec.z,modelMatrix,projMatrix,viewport, \
		&t_zvec.x,&t_zvec.y,&t_zvec.z); \
	gluUnProject(yvec.x,yvec.y,yvec.z,modelMatrix,projMatrix,viewport, \
		&t_yvec.x,&t_yvec.y,&t_yvec.z); \
 \
 \
	/*printf ("\n"); \
	printf ("unprojected, t_orig (0,0,0) %lf %lf %lf\n",t_orig.x, t_orig.y, t_orig.z); \
	printf ("unprojected, t_yvec (0,0.05,0) %lf %lf %lf\n",t_yvec.x, t_yvec.y, t_yvec.z); \
	printf ("unprojected, t_zvec (0,0,-0.05) %lf %lf %lf\n",t_zvec.x, t_zvec.y, t_zvec.z); \
	*/ \
	cx = t_orig.x - ((node->center ).c[0]); \
	cy = t_orig.y - ((node->center ).c[1]); \
	cz = t_orig.z - ((node->center ).c[2]); \
 \
	if(((node->size).c[0]) == 0 || ((node->size).c[1]) == 0 || ((node->size).c[2]) == 0) return; \
 \
	if(fabs(cx) > ((node->size).c[0])/2 || \
	   fabs(cy) > ((node->size).c[1])/2 || \
	   fabs(cz) > ((node->size).c[2])/2) return; \
	/* printf ("within (Geo)ProximitySensor\n"); */ \
 \
	/* Ok, we now have to compute... */ \
	(node->__hit) /*cget*/ = 1; \
 \
	/* Position */ \
	((node->__t1).c[0]) = t_orig.x; \
	((node->__t1).c[1]) = t_orig.y; \
	((node->__t1).c[2]) = t_orig.z; \
 \
	VECDIFF(t_zvec,t_orig,dr1r2);  /* Z axis */ \
	VECDIFF(t_yvec,t_orig,dr2r3);  /* Y axis */ \
 \
	/* printf ("      dr1r2 %lf %lf %lf\n",dr1r2.x, dr1r2.y, dr1r2.z); \
	printf ("      dr2r3 %lf %lf %lf\n",dr2r3.x, dr2r3.y, dr2r3.z); \
	*/ \
 \
	len = sqrt(VECSQ(dr1r2)); VECSCALE(dr1r2,1/len); \
	len = sqrt(VECSQ(dr2r3)); VECSCALE(dr2r3,1/len); \
 \
	/* printf ("scaled dr1r2 %lf %lf %lf\n",dr1r2.x, dr1r2.y, dr1r2.z); \
	printf ("scaled dr2r3 %lf %lf %lf\n",dr2r3.x, dr2r3.y, dr2r3.z); \
	*/ \
 \
	/* \
	printf("PROX_INT: (%f %f %f) (%f %f %f) (%f %f %f)\n (%f %f %f) (%f %f %f)\n", \
		t_orig.x, t_orig.y, t_orig.z, \
		t_zvec.x, t_zvec.y, t_zvec.z, \
		t_yvec.x, t_yvec.y, t_yvec.z, \
		dr1r2.x, dr1r2.y, dr1r2.z, \
		dr2r3.x, dr2r3.y, dr2r3.z \
		); \
	*/ \
 \
	if(fabs(VECPT(dr1r2, dr2r3)) > 0.001) { \
		printf ("Sorry, can't handle unevenly scaled ProximitySensors yet :(" \
		  "dp: %f v: (%f %f %f) (%f %f %f)\n", VECPT(dr1r2, dr2r3), \
		  	dr1r2.x,dr1r2.y,dr1r2.z, \
		  	dr2r3.x,dr2r3.y,dr2r3.z \
			); \
		return; \
	} \
 \
 \
	if(APPROX(dr1r2.z,1.0)) { \
		/* rotation */ \
		((node->__t2).r[0]) = 0; \
		((node->__t2).r[1]) = 0; \
		((node->__t2).r[2]) = 1; \
		((node->__t2).r[3]) = atan2(-dr2r3.x,dr2r3.y); \
	} else if(APPROX(dr2r3.y,1.0)) { \
		/* rotation */ \
		((node->__t2).r[0]) = 0; \
		((node->__t2).r[1]) = 1; \
		((node->__t2).r[2]) = 0; \
		((node->__t2).r[3]) = atan2(dr1r2.x,dr1r2.z); \
	} else { \
		/* Get the normal vectors of the possible rotation planes */ \
		nor1 = dr1r2; \
		nor1.z -= 1.0; \
		nor2 = dr2r3; \
		nor2.y -= 1.0; \
 \
		/* Now, the intersection of the planes, obviously cp */ \
		VECCP(nor1,nor2,ins); \
 \
		len = sqrt(VECSQ(ins)); VECSCALE(ins,1/len); \
 \
		/* the angle */ \
		VECCP(dr1r2,ins, nor1); \
		VECCP(zpvec, ins, nor2); \
		len = sqrt(VECSQ(nor1)); VECSCALE(nor1,1/len); \
		len = sqrt(VECSQ(nor2)); VECSCALE(nor2,1/len); \
		VECCP(nor1,nor2,ins); \
 \
		((node->__t2).r[3]) = -atan2(sqrt(VECSQ(ins)), VECPT(nor1,nor2)); \
 \
		/* rotation  - should normalize sometime... */ \
		((node->__t2).r[0]) = ins.x; \
		((node->__t2).r[1]) = ins.y; \
		((node->__t2).r[2]) = ins.z; \
	} \
	/* \
	printf("NORS: (%f %f %f) (%f %f %f) (%f %f %f)\n", \
		nor1.x, nor1.y, nor1.z, \
		nor2.x, nor2.y, nor2.z, \
		ins.x, ins.y, ins.z \
	); \
	*/ \
} 


#endif /* __FREEX3D_HEADERS_H__ */