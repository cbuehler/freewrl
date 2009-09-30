/*
=INSERT_TEMPLATE_HERE=

$Id: Component_ProgrammableShaders.c,v 1.25 2009/09/30 20:08:49 crc_canada Exp $

X3D Programmable Shaders Component

*/

/* Mapping X3D types to shaders

Notes:
	If the shader variable is not defined AND used in the shader, you will get an error on initialization
	via X3D. Even if it is defined as a Uniform variable, if it does not exist in the shader, it will not 
	be able to be initialized, and thus the shader interface will return an error.


X3D type			GLSL type		Initialize	Route In	Route Out
									(uniform variables only)
-------------------------------------------------------------------------------------------------
FIELDTYPE_SFFloat		GL_FLOAT		YES		YES
FIELDTYPE_MFFloat		GL_FLOAT		Uniform only
FIELDTYPE_SFRotation		GL_FLOAT_VEC4		YES	
FIELDTYPE_MFRotation		GL_FLOAT_VEC4		Uniform only
FIELDTYPE_SFVec3f		GL_FLOAT_VEC3		YES		YES
FIELDTYPE_MFVec3f		GL_FLOAT_VEC3		Uniform only
FIELDTYPE_SFBool	
FIELDTYPE_MFBool	
FIELDTYPE_SFInt32	
FIELDTYPE_MFInt32	
FIELDTYPE_SFNode		GL_INT	
FIELDTYPE_MFNode		--
FIELDTYPE_SFColor		GL_FLOAT_VEC3		YES
FIELDTYPE_MFColor		GL_FLOAT_VEC3		Uniform only
FIELDTYPE_SFColorRGBA		GL_FLOAT_VEC4		YES
FIELDTYPE_MFColorRGBA		GL_FLOAT_VEC4		Uniform only
FIELDTYPE_SFTime		GL_FLOAT		YES(float)
FIELDTYPE_MFTime	
FIELDTYPE_SFString		--
FIELDTYPE_MFString		--
FIELDTYPE_SFVec2f		GL_FLOAT_VEC2		YES
FIELDTYPE_MFVec2f		GL_FLOAT_VEC2		Uniform only
FIELDTYPE_SFImage	
FIELDTYPE_FreeWRLPTR		--
FIELDTYPE_SFVec3d		GL_FLOAT_VEC3		YES(float)
FIELDTYPE_MFVec3d	
FIELDTYPE_SFDouble		GL_FLOAT		YES(float)
FIELDTYPE_MFDouble	
FIELDTYPE_SFMatrix3f	
FIELDTYPE_MFMatrix3f	
FIELDTYPE_SFMatrix3d	
FIELDTYPE_MFMatrix3d	
FIELDTYPE_SFMatrix4f	
FIELDTYPE_MFMatrix4f	
FIELDTYPE_SFMatrix4d	
FIELDTYPE_MFMatrix4d	
FIELDTYPE_SFVec2d		GL_FLOAT_2		YES(float)
FIELDTYPE_MFVec2d	
FIELDTYPE_SFVec4f		GL_FLOAT_VEC4		YES
FIELDTYPE_MFVec4f				
FIELDTYPE_SFVec4d		GL_FLOAT_VEC4		YES(float)
FIELDTYPE_MFVec4d	
*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../lib/scenegraph/Vector.h"

#include "../world_script/CScripts.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../opengl/OpenGL_Utils.h"
#include "Component_ProgrammableShaders.h"


/* which shader is running?? */
GLuint globalCurrentShader = 0;
static void sendInitialFieldsToShader(struct X3D_Node *);

#define MAX_INFO_LOG_SIZE 512
/* we do support older versions of shaders; but not all info logs are printed if we
   have OpenGL prior to 2.0 */


/* Versions 1.5 and above have shaders */
#ifdef GL_VERSION_2_0
	#define HAVE_SHADERS
	#define VERTEX_SHADER GL_VERTEX_SHADER
	#define FRAGMENT_SHADER GL_FRAGMENT_SHADER
	#define SHADER_SOURCE glShaderSource
	#define COMPILE_SHADER glCompileShader
	#define CREATE_PROGRAM glCreateProgram();
	#define ATTACH_SHADER glAttachShader
	#define LINK_SHADER glLinkProgram
	#define USE_SHADER glUseProgram
	#define CREATE_SHADER glCreateShader
	#define GET_SHADER_INFO glGetShaderiv
	#define LINK_STATUS GL_LINK_STATUS
	#define COMPILE_STATUS GL_COMPILE_STATUS
	#define GET_UNIFORM(aaa,bbb) glGetUniformLocation(aaa,bbb)
	#define GET_ATTRIB(aaa,bbb) glGetAttribLocation(aaa,bbb)
	#define GLUNIFORM1F glUniform1f
	#define GLUNIFORM2F glUniform2f
	#define GLUNIFORM3F glUniform3f
	#define GLUNIFORM4F glUniform4f
	#define GLUNIFORM1FV glUniform1fv
	#define GLUNIFORM2FV glUniform2fv
	#define GLUNIFORM3FV glUniform3fv
	#define GLUNIFORM4FV glUniform4fv
	#define GLATTRIB1F glVertexAttrib1f
	#define GLATTRIB2F glVertexAttrib2f
	#define GLATTRIB3F glVertexAttrib3f
	#define GLATTRIB4F glVertexAttrib4f
	#define GLATTRIB1FV glVertexAttrib1fv
	#define GLATTRIB2FV glVertexAttrib2fv
	#define GLATTRIB3FV glVertexAttrib3fv
	#define GLATTRIB4FV glVertexAttrib4fv
#else
#ifdef GL_VERSION_1_5
	#define HAVE_SHADERS
	#define VERTEX_SHADER GL_VERTEX_SHADER_ARB
	#define FRAGMENT_SHADER GL_FRAGMENT_SHADER_ARB
	#define SHADER_SOURCE glShaderSourceARB
	#define COMPILE_SHADER glCompileShaderARB
	#define CREATE_PROGRAM glCreateProgramObjectARB();
	#define ATTACH_SHADER glAttachObjectARB
	#define LINK_SHADER glLinkProgramARB
	#define USE_SHADER  glUseProgramObjectARB
	#define CREATE_SHADER glCreateShaderObjectARB
	#define GET_SHADER_INFO glGetObjectParameterivARB
	#define LINK_STATUS GL_OBJECT_LINK_STATUS_ARB
	#define COMPILE_STATUS GL_OBJECT_COMPILE_STATUS_ARB
	#define GET_UNIFORM(aaa,bbb) glGetUniformLocationARB(aaa,bbb)
	#define GET_ATTRIB(aaa,bbb) glGetAttribLocationARB(aaa,bbb)
	#define GLUNIFORM1F glUniform1fARB
	#define GLUNIFORM2F glUniform2fARB
	#define GLUNIFORM3F glUniform3fARB
	#define GLUNIFORM4F glUniform4fARB
	#define GLUNIFORM1FV glUniform1fvARB
	#define GLUNIFORM2FV glUniform2fvARB
	#define GLUNIFORM3FV glUniform3fvARB
	#define GLUNIFORM4FV glUniform4fvARB
	#define GLATTRIB1F glVertexAttrib1fARB
	#define GLATTRIB2F glVertexAttrib2fARB
	#define GLATTRIB3F glVertexAttrib3fARB
	#define GLATTRIB4F glVertexAttrib4fARB
	#define GLATTRIB1FV glVertexAttrib1fvARB
	#define GLATTRIB2FV glVertexAttrib2fvARB
	#define GLATTRIB3FV glVertexAttrib3fvARB
	#define GLATTRIB4FV glVertexAttrib4fvARB
#endif
#endif

#define SUPPORT_GLSL_ONLY \
	if (strcmp(node->language->strptr,"GLSL")) { \
		ConsoleMessage ("Shaders: support only GLSL shading language, got :%s:, skipping...",node->language->strptr); \
		node->isValid = FALSE; \
	}

#define RUN_IF_VALID \
		if (node->isValid) { \
			if (node->__shaderIDS.n != 0) { \
				globalCurrentShader = (GLuint) node->__shaderIDS.p[0]; \
				USE_SHADER(globalCurrentShader); \
				if (!node->__initialized) sendInitialFieldsToShader(X3D_NODE(node)); \
			} \
		}

#define CHECK_SHADERS \
	if (!GLEW_ARB_fragment_shader) { \
		if (node->isValid) ConsoleMessage ("have an X3D program with shaders, but no shader support on this computer"); \
		node->isValid = FALSE; \
		return; \
	}

static void shaderErrorLog(GLuint myShader) {
	#ifdef GL_VERSION_2_0 
		GLchar infoLog[MAX_INFO_LOG_SIZE]; 
		glGetShaderInfoLog(myShader, MAX_INFO_LOG_SIZE, NULL, infoLog); 
		ConsoleMessage ("problem with VERTEX shader: %s",infoLog); 
	#else 
		ConsoleMessage ("Problem compiling shader"); 
	#endif 
}


/* common code to compile and link shaders */
#define COMPILE_IF_VALID(subField)  \
	if (node->isValid) { \
		GLint success; \
		GLuint myVertexShader = 0; \
		GLuint myFragmentShader= 0; \
		 \
 \
		if (haveVertShaderText) { \
			myVertexShader = CREATE_SHADER (VERTEX_SHADER);	 \
			SHADER_SOURCE(myVertexShader, node->subField.n, (const GLchar **) vertShaderSource, NULL); \
			COMPILE_SHADER(myVertexShader); \
			GET_SHADER_INFO(myVertexShader, COMPILE_STATUS, &success); \
			if (!success) { \
				shaderErrorLog(myVertexShader); \
				node->isValid = FALSE; \
			} else { \
				ATTACH_SHADER(myProgram, myVertexShader); \
			} \
		} \
		 \
		if (haveFragShaderText) {	 \
			myFragmentShader = CREATE_SHADER (FRAGMENT_SHADER);	 \
			SHADER_SOURCE(myFragmentShader, node->subField.n,(const GLchar **)  fragShaderSource, NULL); \
			COMPILE_SHADER(myFragmentShader); \
 \
			GET_SHADER_INFO(myFragmentShader, COMPILE_STATUS, &success); \
			if (!success) { \
				shaderErrorLog(myFragmentShader); \
				node->isValid = FALSE; \
			} else { \
				ATTACH_SHADER(myProgram, myFragmentShader); \
			} \
		} \
	}

#define COMPILE_SHADER_PARTS(myNodeType, myField) \
		for (i=0; i<node->myField.n; i++) { \
			struct X3D_##myNodeType *prog; \
			prog = (struct X3D_##myNodeType *) node->myField.p[i]; \
			vertShaderSource[i] = ""; \
			fragShaderSource[i] = ""; \
 \
			if (prog!=NULL) { \
				if (prog->_nodeType == NODE_##myNodeType) { \
					/* compile this program */ \
 \
					if (!((strcmp (prog->type->strptr,"VERTEX")) && (strcmp(prog->type->strptr,"FRAGMENT")))) { \
						char *myText = NULL; /* pointer to text to send in */ \
						char **cptr; /* returned caracter pointer pointer */ \
						 \
						  cptr = shader_initCodeFromMFUri(&prog->url); \
						  if (cptr == NULL) { \
							ConsoleMessage ("error reading url for :%s:",stringNodeType(NODE_##myNodeType)); \
							myText = ""; \
						  } else { myText = *cptr; \
\
						/* assign this text to VERTEX or FRAGMENT buffers */ \
						if (!strcmp(prog->type->strptr,"VERTEX")) { \
							vertShaderSource[i] = STRDUP(myText); \
							haveVertShaderText = TRUE; \
						} else { \
							fragShaderSource[i] = STRDUP(myText); \
							haveFragShaderText = TRUE; \
						} \
						/* printf ("Shader text for type %s is  %s\n",prog->type->strptr,myText); */ \
						FREE_IF_NZ(*cptr); }\
					} else { \
						ConsoleMessage ("%s, invalid Type, got \"%s\"",stringNodeType(NODE_##myNodeType), prog->type->strptr); \
						node->isValid = FALSE; \
					} \
				} else { \
					ConsoleMessage ("Shader, expected \"%s\", got \"%s\"",stringNodeType(NODE_##myNodeType), stringNodeType(prog->_nodeType)); \
					node->isValid = FALSE; \
				} \
			} \
		} 

#ifdef GL_VERSION_2_0
	#define LINK_IF_VALID \
		if (node->isValid) { \
			GLint success; \
			/* link the shader programs together */ \
			LINK_SHADER(myProgram); \
			glGetProgramiv(myProgram, GL_LINK_STATUS, &success); \
			if (!success) { \
				GLchar infoLog[MAX_INFO_LOG_SIZE]; \
				glGetProgramInfoLog(myProgram, MAX_INFO_LOG_SIZE, NULL, infoLog); \
				printf ("problem with Shader Program link: %s\n",infoLog); \
				node->isValid = FALSE; \
			} \
			/* does the program get a thumbs up? */	 \
			glValidateProgram (myProgram); \
			glGetProgramiv(myProgram, GL_VALIDATE_STATUS, &success); \
			if (!success) { \
				GLchar infoLog[MAX_INFO_LOG_SIZE]; \
				glGetProgramInfoLog(myProgram, MAX_INFO_LOG_SIZE, NULL, infoLog); \
				printf ("problem with Shader Program Validate: %s\n",infoLog); \
				node->isValid = FALSE; \
			} \
			if (node->__shaderIDS.n == 0) { \
				node->__shaderIDS.n = 1; \
				node->__shaderIDS.p = MALLOC(sizeof (GLuint)); \
				node->__shaderIDS.p[0] = (int)myProgram; \
			} \
		}
#else
	#define LINK_IF_VALID \
		if (node->isValid) { \
			/* link the shader programs together */ \
			LINK_SHADER(myProgram); \
			if (node->__shaderIDS.n == 0) { \
				node->__shaderIDS.n = 1; \
				node->__shaderIDS.p = MALLOC(sizeof (GLuint)); \
				node->__shaderIDS.p[0] = (int)myProgram; \
			} \
		}
#endif

/* do type checking of shader and field variables when initializing interface */
static int shader_checkType(struct FieldDecl * myField,
		GLuint myShader, int isUniform, GLint myVar) {
	int retval;

	/* check the type, if we are OpenGL 2.0 or above */

	#ifdef GL_VERSION_2_0
	GLsizei len;
	GLint size;
	GLenum type;
	GLchar ch[100];		

	retval = FALSE;
	ch[0] = '\0';
	
	if (isUniform)	glGetActiveUniform (myShader,myVar,90,&len,&size,&type,ch);
	else glGetActiveAttrib (myShader,myVar,90,&len,&size,&type,ch);


	/* verify that the X3D fieldType matches the Shader type */
	switch (myField->type) {
		case FIELDTYPE_SFFloat: 	retval = type == GL_FLOAT; break;
		case FIELDTYPE_MFFloat: 	retval = type == GL_FLOAT; break;
		case FIELDTYPE_SFRotation: 	retval = type == GL_FLOAT_VEC4; break;
		case FIELDTYPE_MFRotation: 	retval = type == GL_FLOAT_VEC4; break;
		case FIELDTYPE_SFVec3f: 	retval = type == GL_FLOAT_VEC3; break;
		case FIELDTYPE_MFVec3f: 	retval = type == GL_FLOAT_VEC3; break;
		case FIELDTYPE_SFBool: 		break;
		case FIELDTYPE_MFBool: 		break;
		case FIELDTYPE_SFInt32: 	break;
		case FIELDTYPE_MFInt32: 	break;
		case FIELDTYPE_SFNode: 		break;
		case FIELDTYPE_MFNode: 		break;
		case FIELDTYPE_SFColor: 	retval = type == GL_FLOAT_VEC3; break;
		case FIELDTYPE_MFColor: 	retval = type == GL_FLOAT_VEC3; break;
		case FIELDTYPE_SFColorRGBA: 	retval = type == GL_FLOAT_VEC4; break;
		case FIELDTYPE_MFColorRGBA: 	retval = type == GL_FLOAT_VEC4; break;
		case FIELDTYPE_SFTime: 		retval = type == GL_FLOAT; break;
		case FIELDTYPE_MFTime: 		break;
		case FIELDTYPE_SFString: 	break;
		case FIELDTYPE_MFString: 	break;
		case FIELDTYPE_SFVec2f: 	retval = type ==  GL_FLOAT_VEC2; break;
		case FIELDTYPE_MFVec2f: 	retval = type ==  GL_FLOAT_VEC2; break;
		case FIELDTYPE_SFImage: 	break;
		case FIELDTYPE_FreeWRLPTR: 	break;
		case FIELDTYPE_SFVec3d: 	retval = type == GL_FLOAT_VEC3; break;
		case FIELDTYPE_MFVec3d: 	break;
		case FIELDTYPE_SFDouble: 	retval = type ==  GL_FLOAT; break;
		case FIELDTYPE_MFDouble:	break;
		case FIELDTYPE_SFMatrix3f:	break;
		case FIELDTYPE_MFMatrix3f:	break;
		case FIELDTYPE_SFMatrix3d:	break;
		case FIELDTYPE_MFMatrix3d:	break;
		case FIELDTYPE_SFMatrix4f:	break;
		case FIELDTYPE_MFMatrix4f:	break;
		case FIELDTYPE_SFMatrix4d:	break;
		case FIELDTYPE_MFMatrix4d: 	break;
		case FIELDTYPE_SFVec2d: 	retval = type == GL_FLOAT_VEC2; break;
		case FIELDTYPE_MFVec2d: 	break;
		case FIELDTYPE_SFVec4f: 	retval = type == GL_FLOAT_VEC4; break;
		case FIELDTYPE_MFVec4f: 	break;
		case FIELDTYPE_SFVec4d: 	retval = type == GL_FLOAT_VEC4; break;
		case FIELDTYPE_MFVec4d: 	break;
	}

	/* did we have an error? */

	if (!retval) {
		ConsoleMessage ("Shader type check fail X3D type not compatible for variable :%s:",ch);
#ifdef VERBOSE
	printf ("shaderCheck mode %d (%s) type %d (%s) name %d\n",myField->mode, 
			stringPROTOKeywordType(myField->mode), myField->type, stringFieldtypeType(myField->type),myField->name);

	printf ("len %d size %d type %d ch %s\n",len,size,type,ch);
	switch (type) {

	case GL_FLOAT: printf ("GL_FLOAT\n"); break;
	case GL_FLOAT_VEC2: printf ("GL_FLOAT_VEC2\n"); break;
	case GL_FLOAT_VEC3: printf ("GL_FLOAT_VEC3\n"); break;
	case GL_FLOAT_VEC4: printf ("GL_FLOAT_VEC4\n"); break;
	case GL_INT: printf ("GL_INT\n"); break;
	case GL_INT_VEC2: printf ("GL_INT_VEC2\n"); break;
	case GL_INT_VEC3: printf ("GL_INT_VEC3\n"); break;
	case GL_INT_VEC4: printf ("GL_INT_VEC4\n"); break;
	case GL_BOOL: printf ("GL_BOOL\n"); break;
	case GL_BOOL_VEC2: printf ("GL_BOOL_VEC2\n"); break;
	case GL_BOOL_VEC3: printf ("GL_BOOL_VEC3\n"); break;
	case GL_BOOL_VEC4: printf ("GL_BOOL_VEC4\n"); break;
	case GL_FLOAT_MAT2: printf ("GL_FLOAT_MAT2\n"); break;
	case GL_FLOAT_MAT3: printf ("GL_FLOAT_MAT3\n"); break;
	case GL_FLOAT_MAT4: printf ("GL_FLOAT_MAT4\n"); break;
	case GL_FLOAT_MAT2x3: printf ("GL_FLOAT_MAT2x3\n"); break;
	case GL_FLOAT_MAT2x4: printf ("GL_FLOAT_MAT2x4\n"); break;
	case GL_FLOAT_MAT3x2: printf ("GL_FLOAT_MAT3x2\n"); break;
	case GL_FLOAT_MAT3x4: printf ("GL_FLOAT_MAT3x4\n"); break;
	case GL_FLOAT_MAT4x2: printf ("GL_FLOAT_MAT4x2\n"); break;
	case GL_FLOAT_MAT4x3: printf ("GL_FLOAT_MAT4x3\n"); break;
	case GL_SAMPLER_1D: printf ("GL_SAMPLER_1D\n"); break;
	case GL_SAMPLER_2D: printf ("GL_SAMPLER_2D\n"); break;
	case GL_SAMPLER_3D: printf ("GL_SAMPLER_3D\n"); break;
	case GL_SAMPLER_CUBE: printf ("GL_SAMPLER_CUBE\n"); break;
	case GL_SAMPLER_1D_SHADOW: printf ("GL_SAMPLER_1D_SHADOW\n"); break;
	case GL_SAMPLER_2D_SHADOW: printf ("GL_SAMPLER_2D_SHADOW\n"); break;
	}
#endif
	}
#endif 
	return retval;
}


/* fieldDecl_getshaderVariableID(myf), fieldDecl_getValue(myf)); */
static void sendValueToShader(struct ScriptFieldDecl* myField) {
	GLint shaderVariable = fieldDecl_getshaderVariableID(myField->fieldDecl);
	int isUniform = fieldDecl_isshaderVariableUniform(myField->fieldDecl);

	#ifdef SHADERVERBOSE
	printf ("sendValueToShader... ft %s\n",stringFieldtypeType(fieldDecl_getType(myField->fieldDecl)));
	printf ("shaderVariableID is %d\n",shaderVariable);
	#endif

	/* either not defined in the shader, OR not used in the shader so it is stripped by glsl compiler */
	if (shaderVariable == INT_ID_UNDEFINED) return;

	switch (fieldDecl_getType(myField->fieldDecl)) {

#define SF_FLOATS_TO_SHADER(ttt,ty1,ty2) \
		case FIELDTYPE_SF##ty1: \
			if (isUniform) \
				GLUNIFORM##ttt##FV(shaderVariable, 1, myField->value.sf##ty2.c); \
			else \
				GLATTRIB##ttt##FV(shaderVariable, myField->value.sf##ty2.c); \
		break; 

#define SF_DOUBLES_TO_SHADER(ttt,ty1,ty2) \
		case FIELDTYPE_SF##ty1: {float val[4]; int i; \
			for (i=0; i<ttt; i++) { val[i] = (float) (myField->value.sf##ty2.c[i]); } \
			if (isUniform) \
				GLUNIFORM##ttt##FV(shaderVariable, 1, val); \
			else \
				GLATTRIB##ttt##FV(shaderVariable, val); \
		break; }

#define SF_FLOAT_TO_SHADER(ty1,ty2) \
		case FIELDTYPE_SF##ty1: \
			if (isUniform) \
				GLUNIFORM1F(shaderVariable, myField->value.sf##ty2); \
			else \
				GLATTRIB1F(shaderVariable, myField->value.sf##ty2); \
		break; 

#define SF_DOUBLE_TO_SHADER(ty1,ty2) \
		case FIELDTYPE_SF##ty1: {float val = myField->value.sf##ty2; \
			if (isUniform) \
				GLUNIFORM1F(shaderVariable, val); \
			else \
				GLATTRIB1F(shaderVariable, val); \
		break; }

#define MF_FLOATS_TO_SHADER(ttt,ty1,ty2) \
		case FIELDTYPE_MF##ty1: \
			if (isUniform) \
				GLUNIFORM##ttt##FV(shaderVariable, myField->value.mf##ty2.n, (float *)myField->value.mf##ty2.p); \
			else \
				ConsoleMessage ("problem sending MF to Shader Attribute"); \
		break; 


		SF_FLOAT_TO_SHADER(Float,float)
		SF_DOUBLE_TO_SHADER(Double,float)
		SF_DOUBLE_TO_SHADER(Time,float)

		SF_FLOATS_TO_SHADER(2,Vec2f,vec2f)
		SF_FLOATS_TO_SHADER(3,Vec3f,vec3f)
		SF_FLOATS_TO_SHADER(3,Color,color)
		SF_FLOATS_TO_SHADER(4,ColorRGBA,colorrgba)
		SF_FLOATS_TO_SHADER(4,Rotation,rotation)
		SF_FLOATS_TO_SHADER(4,Vec4f,vec4f)
		SF_DOUBLES_TO_SHADER(2,Vec2d, vec2d)
		SF_DOUBLES_TO_SHADER(3,Vec3d, vec3d)
		SF_DOUBLES_TO_SHADER(4,Vec4d, vec4d)

		MF_FLOATS_TO_SHADER(1,Float,float)
		MF_FLOATS_TO_SHADER(2,Vec2f,vec2f)
		MF_FLOATS_TO_SHADER(3,Color,color)
		MF_FLOATS_TO_SHADER(3,Vec3f,vec3f)
		MF_FLOATS_TO_SHADER(4,ColorRGBA,colorrgba)
		MF_FLOATS_TO_SHADER(4,Rotation,rotation)

		//MF_FLOATS_TO_SHADER(4,Vec4f,vec4f)

		//SF_FLOAT_TO_SHADER(9,Matrix3f, matrix3f)
		//SF_FLOAT_TO_SHADER(16,Matrix4f, matrix4f)

		case FIELDTYPE_SFNode:
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFImage:
		case FIELDTYPE_FreeWRLPTR:
		case FIELDTYPE_SFString:
		case FIELDTYPE_SFMatrix3d:
		case FIELDTYPE_SFMatrix4d:


		case FIELDTYPE_MFBool:
		case FIELDTYPE_MFInt32:
		case FIELDTYPE_MFNode:
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFString:
		case FIELDTYPE_MFDouble:
		case FIELDTYPE_MFVec3d:
		case FIELDTYPE_MFMatrix3f:
		case FIELDTYPE_MFMatrix3d:
		case FIELDTYPE_MFMatrix4f:
		case FIELDTYPE_MFMatrix4d:
		case FIELDTYPE_MFVec2d:
		case FIELDTYPE_MFVec4d:
		default : printf ("can not send a shader variable of %s yet\n",stringFieldtypeType(fieldDecl_getType(myField->fieldDecl)));
	}
}

/* Routing - sending a value to a shader - we get passed the routing table entry number */
void getField_ToShader(int num) {
	struct Shader_Script *myObj;
	unsigned int to_counter;
	int fromFieldID;
	size_t i;
	GLfloat* sourceData;
	GLuint currentShader;	


	/* go through each destination for this node */
	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
		CRnodeStruct *to_ptr = NULL;

		to_ptr = &(CRoutes[num].tonodes[to_counter]);
		fromFieldID = to_ptr->foffset;
		/* printf ("getField_ToShader, num %d, foffset %d\n",num,fromFieldID); */

		switch (to_ptr->routeToNode->_nodeType) {
		case NODE_ComposedShader:
			myObj = X3D_COMPOSEDSHADER(to_ptr->routeToNode)->__shaderObj;
			break;
		case NODE_PackagedShader:
			myObj = X3D_PACKAGEDSHADER(to_ptr->routeToNode)->__shaderObj;
			break;
		case NODE_ShaderProgram: 
			myObj = X3D_SHADERPROGRAM(to_ptr->routeToNode)->__shaderObj;
			break;
		default: {
			ConsoleMessage ("getField_ToShader, unhandled type??");
			return;
			}

		}
		/* we have the struct Shader_Script; go through the fields and find the correct one */

/*
	printf ("Shader_Script has node of %u ",myObj->ShaderScriptNode);
	printf ("of type %s ",stringNodeType(myObj->ShaderScriptNode->_nodeType));
	printf ("and has %d as a vector size ",vector_size(myObj->fields));
	if (myObj->loaded) printf ("locked and loaded "); else printf ("needs loading, I guess ");
	printf ("\n");
*/

	/* is there any fields? */
	if (myObj == NULL) return;

	/* this script should be loaded... if not, wait until it is */
	if (!myObj->loaded) {
		/* ConsoleMessage ("ShaderProgram should be loaded, hmmm"); */
		return;
	}

	switch (to_ptr->routeToNode->_nodeType) {
	case NODE_ComposedShader:
		currentShader = (GLuint) X3D_COMPOSEDSHADER(to_ptr->routeToNode)->__shaderIDS.p[0];
		break;
	case NODE_PackagedShader:
		ConsoleMessage ("do not know how to route to a PackagedShader yet");
		return;
		break;
	case NODE_ShaderProgram: 
		/* printf ("ShaderProgram- the parent ProgramShader holds the ids\n"); */
		if (X3D_SHADERPROGRAM(to_ptr->routeToNode)->_nparents > 0) {
			/* assume only one parent here? */
			currentShader = (GLuint) X3D_PROGRAMSHADER(
				X3D_SHADERPROGRAM(to_ptr->routeToNode)->_parents[0])->__shaderIDS.p[0];
		} else {
			printf ("no parents for routed ShaderProgram\n");
		}

		break;
	}


	for(i=0; i!=vector_size(myObj->fields); ++i) {
		int isUniform; 
		GLint myVar;
		struct ScriptFieldDecl* curField;
		struct FieldDecl * myf;

		/* initialization */
		curField = vector_get(struct ScriptFieldDecl*, myObj->fields, i);
		myf = curField->fieldDecl;

		/*
		printf ("curField %d name %d type %d ",i,myf->name,myf->type);
		printf ("fieldDecl mode %d (%s) type %d (%s) name %d\n",myf->mode, 
			stringPROTOKeywordType(myf->mode), myf->type, stringFieldtypeType(myf->type),myf->name);
		*/



		if (fromFieldID == myf->name) {
			/*
			printf ("	field match, %d==%d\n",fromFieldID, myf->name);
			printf ("	types %d, %d\n",JSparamnames[fromFieldID].type,myf->type);
			printf ("	shaderVariableID is %d\n",myf->shaderVariableID);
			printf ("	shaderVariableIsUniform %d\n",myf->shaderVariableIsUniform);
			*/

		/* ok, here we have the Shader_Script, the field offset, and the entry */

		sourceData = offsetPointer_deref(GLfloat *,CRoutes[num].routeFromNode, CRoutes[num].fnptr);
		
		/* turn the selected shader program ON */
		USE_SHADER(currentShader);

		/* send in the correct parameters */
		switch (JSparamnames[fromFieldID].type) {
			case FIELDTYPE_SFFloat:
				GLUNIFORM1FV(myf->shaderVariableID, 1, sourceData);
				break;

			case FIELDTYPE_SFVec2f:
				GLUNIFORM2FV(myf->shaderVariableID, 1, sourceData);
				break;

			case FIELDTYPE_SFVec3f:
			case FIELDTYPE_SFColor:
				GLUNIFORM3FV(myf->shaderVariableID, 1, sourceData);
				break;

			case FIELDTYPE_SFColorRGBA:
			case FIELDTYPE_SFRotation:
			case FIELDTYPE_SFVec4f:
				GLUNIFORM4FV(myf->shaderVariableID, 1, sourceData);
				break;

			case FIELDTYPE_MFFloat:
			case FIELDTYPE_MFRotation:
			case FIELDTYPE_MFVec3f:
			case FIELDTYPE_SFBool:
			case FIELDTYPE_MFBool:
			case FIELDTYPE_SFInt32:
			case FIELDTYPE_MFInt32:
			case FIELDTYPE_SFNode:
			case FIELDTYPE_MFNode:
			case FIELDTYPE_MFColor:
			case FIELDTYPE_MFColorRGBA:
			case FIELDTYPE_SFTime:
			case FIELDTYPE_MFTime:
			case FIELDTYPE_SFString:
			case FIELDTYPE_MFString:
			case FIELDTYPE_MFVec2f:
			case FIELDTYPE_SFImage:
			case FIELDTYPE_FreeWRLPTR:
			case FIELDTYPE_SFVec3d:
			case FIELDTYPE_MFVec3d:
			case FIELDTYPE_SFDouble:
			case FIELDTYPE_MFDouble:
			case FIELDTYPE_SFMatrix3f:
			case FIELDTYPE_MFMatrix3f:
			case FIELDTYPE_SFMatrix3d:
			case FIELDTYPE_MFMatrix3d:
			case FIELDTYPE_SFMatrix4f:
			case FIELDTYPE_MFMatrix4f:
			case FIELDTYPE_SFMatrix4d:
			case FIELDTYPE_MFMatrix4d:
			case FIELDTYPE_SFVec2d:
			case FIELDTYPE_MFVec2d:
			case FIELDTYPE_MFVec4f:
			case FIELDTYPE_SFVec4d:
			case FIELDTYPE_MFVec4d:
				ConsoleMessage ("shader field type i%f not routable yet",stringFieldtypeType(myf->type));
				break;
			default: {
				ConsoleMessage ("shader field type i%f not routable yet",stringFieldtypeType(myf->type));
			}


			}

			/* turn the shader OFF */
			USE_SHADER(0);
		}
	}
	}
}

/* send fields to a shader; expect either a ShaderProgram, or a ComposedShader */
static void send_fieldToShader (GLuint myShader, struct X3D_Node *node) {
	size_t i;

	struct Shader_Script* me = NULL;
	if (node->_nodeType==NODE_ShaderProgram) me = (struct Shader_Script *) X3D_SHADERPROGRAM(node)->__shaderObj;
	else if (node->_nodeType == NODE_ComposedShader) me = (struct Shader_Script *) X3D_COMPOSEDSHADER(node)->__shaderObj;
	else {
		printf ("send_fieldToShader, expected a ShaderProgram or ComposedShader, got %s\n",
			stringNodeType(node->_nodeType));
		return;
	}

	#ifdef SHADERVERBOSE
	printf ("Shader_Script has node of %u ",me->ShaderScriptNode);
	printf ("of type %s ",stringNodeType(me->ShaderScriptNode->_nodeType));
	printf ("and has %d as a vector size ",vector_size(me->fields));
	if (me->loaded) printf ("locked and loaded "); else printf ("needs loading, I guess ");
	printf ("\n");
	#endif

	/* is there any fields? */
	if (me == NULL) return;

	/* this script should NOT be loaded... */
	if (me->loaded) ConsoleMessage ("ShaderProgram is flagged as being loaded, hmmm");


	for(i=0; i!=vector_size(me->fields); ++i) {
		int isUniform; 
		GLint myVar;
		struct ScriptFieldDecl* curField;
		struct FieldDecl * myf;

		/* initialization */
		myVar = -1;
		isUniform = TRUE;
		curField = vector_get(struct ScriptFieldDecl*, me->fields, i);
		myf = curField->fieldDecl;

		#ifdef SHADERVERBOSE
		printf ("curField %d name %s type %s ",i,curField->name,curField->type);
		printf ("fieldDecl mode %d (%s) type %d (%s) name %d\n",myf->mode, 
			stringPROTOKeywordType(myf->mode), myf->type, stringFieldtypeType(myf->type),myf->name);
		#endif

		/* ask the shader for its handle for this variable */

		/* try Uniform variables first */
		myVar = GET_UNIFORM(myShader,curField->ASCIIname);
		if (myVar == INT_ID_UNDEFINED) {
			myVar = GET_ATTRIB(myShader,curField->ASCIIname);
			isUniform = FALSE;
		}

		fieldDecl_setshaderVariableUniform(myf,isUniform);

		#ifdef SHADERVERBOSE
		printf ("trying to get ID for :%s:\n",curField->name);
		#endif

		/* do the types of the field variable, and the shader variable match? */
		shader_checkType(myf,myShader,isUniform,myVar);

			
		/* save the variable object for this variable */
		fieldDecl_setshaderVariableID(myf,myVar);

		if ((myf->mode==PKW_initializeOnly) || (myf->mode==PKW_inputOutput)) {
			#ifdef SHADERVERBOSE
			printf ("initializing Shader %d variable %s, type %s\n",myShader, curField->name,stringFieldtypeType(myf->type));
			#endif

			sendValueToShader(curField);
		}

	}

	/* done the loading of this shader part */
	me->loaded = TRUE;
}


/* on load of shader, send along any initial field values in the X3D file to the Shader */
/* we can have shaders like:

ComposedShader { 
	field SFFloat blubber 0.5
	field SFVec3f decis 0.96 0.44 0.22 
	language "GLSL" parts [ 
		ShaderPart { type "VERTEX" url "toon.vs" } 
		ShaderPart { type "FRAGMENT" url "toon.fs" } 
	] }

ProgramShader { 
	language  "GLSL" programs   [ 
		ShaderProgram { 
			field SFFloat blubber 0.5
			type "VERTEX" url "toon.vs" } 
		ShaderProgram { 
			field SFVec3f decis 0.96 0.44 0.22 
			type "FRAGMENT" url "toon.fs" } 
	] }

Note the differing location of the fields...

*/


static void sendInitialFieldsToShader(struct X3D_Node * node) {
	int i;
	GLuint myShader;

	switch (node->_nodeType) {
		case NODE_ProgramShader: {
			/* anything to do here? */ 
			if ((X3D_PROGRAMSHADER(node)->__shaderIDS.n) >=1) {
				myShader = X3D_PROGRAMSHADER(node)->__shaderIDS.p[0];
				for (i=0; i<X3D_PROGRAMSHADER(node)->programs.n; i++) {
					#ifdef SHADERVERBOSE
					printf ("ProgramShader, activate %d isSelected %d isValid %d TRUE %d FALSE %d\n",
						X3D_PROGRAMSHADER(node)->activate,X3D_PROGRAMSHADER(node)->isSelected,
						X3D_PROGRAMSHADER(node)->isValid, TRUE, FALSE);
					printf ("runningShader %d, myShader %d\n",globalCurrentShader, X3D_PROGRAMSHADER(node)->__shaderIDS.p[0]);
					#endif

					struct X3D_ShaderProgram *part = X3D_PROGRAMSHADER(node)->programs.p[i];

					#ifdef SHADERVERBOSE
					printf ("sendInitial, have part %d\n",part);
					#endif

					send_fieldToShader(myShader, X3D_NODE(part));
				}
			}
			X3D_PROGRAMSHADER(node)->__initialized = TRUE;
			break;
		}


		case NODE_ComposedShader: {
			/* anything to do here? */ 
			if ((X3D_COMPOSEDSHADER(node)->__shaderIDS.n) >=1) {
				myShader = X3D_COMPOSEDSHADER(node)->__shaderIDS.p[0];
				send_fieldToShader(myShader, X3D_NODE(node));
			}
			X3D_COMPOSEDSHADER(node)->__initialized = TRUE;
			break;
		}
	}
}

/*********************************************************************/

void compile_ComposedShader (struct X3D_ComposedShader *node) {
	#ifdef HAVE_SHADERS
		/* an array of text pointers, should contain shader source */
		GLchar **vertShaderSource;
		GLchar **fragShaderSource;
		int i;
		GLuint myProgram;
		/* do we have anything to compile? */
		int haveVertShaderText; 
		int haveFragShaderText; 

		/* initialization */
		haveVertShaderText = FALSE;
		haveFragShaderText = FALSE;
		myProgram = CREATE_PROGRAM;

		/* can we do shaders at runtime? */
		CHECK_SHADERS
		vertShaderSource = MALLOC(sizeof(GLchar*) * node->parts.n); 
		fragShaderSource = MALLOC(sizeof(GLchar*) * node->parts.n);
	
		/* set this up... set it to FALSE if there are problems */
		node->isValid = TRUE;
	
		/* we support only GLSL here */
		SUPPORT_GLSL_ONLY
		
		/* ok so far, go through the parts */
		COMPILE_SHADER_PARTS(ShaderPart,parts)
	
		/* compile shader, if things are ok to here */
		COMPILE_IF_VALID(parts)
		LINK_IF_VALID

		MARK_NODE_COMPILED
	#endif	
}
void compile_ProgramShader (struct X3D_ProgramShader *node) {
	#ifdef HAVE_SHADERS
		/* an array of text pointers, should contain shader source */
		GLchar **vertShaderSource;
		GLchar **fragShaderSource;
		int i;
		GLuint myProgram;
		/* do we have anything to compile? */
		int haveVertShaderText; 
		int haveFragShaderText; 

		/* initialization */
		haveVertShaderText = FALSE;
		haveFragShaderText = FALSE;
		myProgram = CREATE_PROGRAM;
	
		CHECK_SHADERS
		vertShaderSource = MALLOC(sizeof(GLchar*) * node->programs.n); 
		fragShaderSource = MALLOC(sizeof(GLchar*) * node->programs.n);
	
		/* set this up... set it to FALSE if there are problems */
		node->isValid = TRUE;
	
		/* we support only GLSL here */
		SUPPORT_GLSL_ONLY
			
		/* ok so far, go through the programs */
		COMPILE_SHADER_PARTS(ShaderProgram,programs)
	
		/* compile shader, if things are ok to here */
		COMPILE_IF_VALID(programs)
		LINK_IF_VALID

		MARK_NODE_COMPILED
	#endif	
}
void compile_PackagedShader (struct X3D_PackagedShader *node) {
	#ifdef HAVE_SHADERS
		ConsoleMessage ("found PackagedShader, do not support this structure, as we support only GLSL");
		node->isValid = FALSE;
		MARK_NODE_COMPILED
	#endif
}


/*****************************************************************/
void render_ComposedShader (struct X3D_ComposedShader *node) {
	#ifdef HAVE_SHADERS
		COMPILE_IF_REQUIRED
		RUN_IF_VALID
	#endif
}
void render_PackagedShader (struct X3D_PackagedShader *node) {
	#ifdef HAVE_SHADERS
		COMPILE_IF_REQUIRED
	#endif
}
void render_ProgramShader (struct X3D_ProgramShader *node) {
	#ifdef HAVE_SHADERS
		COMPILE_IF_REQUIRED
		RUN_IF_VALID
	#endif
}
