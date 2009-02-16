/*
=INSERT_TEMPLATE_HERE=

$Id: EAI_C_CommonFunctions.c,v 1.10 2009/02/16 20:00:32 sdumoulin Exp $

???

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/CScripts.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "EAIheaders.h"


/* TODO: clean-up Rewire */
#ifdef REWIRE
# include "Eai_C.h"
# define ADD_PARENT(a,b)
/* FIXME: is there a problem with MALLOC when building with REWIRE ? */
# define MALLOC(a) malloc(a)
#endif

int eaiverbose = TRUE;

#define PST_MF_STRUCT_ELEMENT(type1,type2) \
	case FIELDTYPE_MF##type1: { \
		struct Multi_##type1 *myv; \
		myv = (struct Multi_type1 *) nst; \
		/* printf ("old val p= %u, n = %d\n",myv->p, myv->n); */\
		myv->p = myVal.mf##type2.p; \
		myv->n = myVal.mf##type2.n; \
		/* printf ("PST_MF_STRUCT_ELEMENT, now, element count %d\n",myv->n); */ \
		break; }


#define PST_SF_SIMPLE_ELEMENT(type1,type2,size3) \
	case FIELDTYPE_SF##type1: { \
		memcpy((void*)nst, &myVal.sf##type2, size3); \
		break; }


/* create a structure to hold a string; it has a length, and a string pointer */
struct Uni_String *newASCIIString(char *str) {
	struct Uni_String *retval;
	int len;

	if (eaiverbose) {
	printf ("newASCIIString for :%s:\n",str);
	}

	/* the returning Uni_String is here. Make blank struct */
	retval = MALLOC (sizeof (struct Uni_String));
	len = strlen(str);

	retval->strptr  = MALLOC (sizeof(char) * len+1);
	strncpy(retval->strptr,str,len+1);
	retval->len = len+1;
	retval->touched = 1; /* make it 1, to signal that this is a NEW string. */
	return retval;
}

/* do these strings differ?? If so, copy the new string over the old, and 
touch the touched flag */
void verify_Uni_String(struct  Uni_String *unis, char *str) {
	char *ns;
	char *os;
	int len;

	/* bounds checking */
	if (unis == NULL) {
		printf ("Warning, verify_Uni_String, comparing to NULL Uni_String, %s\n",str);
		return;
	}

	/* are they different? */
	if (strcmp(str,unis->strptr)!= 0) {
		os = unis->strptr;
		len = strlen(str);
		ns = MALLOC (len+1);
		strncpy(ns,str,len+1);
		unis->strptr = ns;
		FREE_IF_NZ (os);
		unis->touched++;
	}
}
		



/* get how many bytes in the type */
int returnElementLength(int type) {
	  switch (type) {
		case FIELDTYPE_MFVec3d:
		case FIELDTYPE_SFVec3d:
		case FIELDTYPE_MFDouble:
		case FIELDTYPE_SFDouble:
		case FIELDTYPE_SFVec4d:
		case FIELDTYPE_SFTime :
    		case FIELDTYPE_MFTime : return sizeof(double); break;
    		case FIELDTYPE_MFInt32: return sizeof(int)   ; break;
		case FIELDTYPE_FreeWRLPTR:
    		case FIELDTYPE_SFNode :
    		case FIELDTYPE_MFNode : return sizeof(void *); break;
	  	default     : {}
	}
	return sizeof(float) ; /* turn into byte count */
}

/* for passing into CRoutes/CRoutes_Register */
/* lengths are either positive numbers, or, if there is a complex type, a negative number. If positive,
   in routing a memcpy is performed; if negative, then some inquiry is required to get correct length
   of both source/dest during routing. */

int returnRoutingElementLength(int type) {
	  switch (type) {
		case FIELDTYPE_SFTime:	
				return sizeof(double); break;
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFString:
		case FIELDTYPE_SFInt32:	return sizeof(int); break;
		case FIELDTYPE_SFFloat:	return sizeof (float); break;
		case FIELDTYPE_SFVec2f:	return sizeof (struct SFVec2f); break;
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor: 	return sizeof (struct SFColor); break;
		case FIELDTYPE_SFVec3d: return sizeof (struct SFVec3d); break;
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:return sizeof (struct SFRotation); break;
		case FIELDTYPE_SFNode:	return sizeof (uintptr_t); break;
		case FIELDTYPE_MFNode:	return -10; break;
		case FIELDTYPE_SFImage:	return -12; break;
		case FIELDTYPE_MFString: 	return -13; break;
		case FIELDTYPE_MFFloat:	return -14; break;
		case FIELDTYPE_MFColorRGBA:
		case FIELDTYPE_MFRotation: return -15; break;
		case FIELDTYPE_MFBool:
		case FIELDTYPE_MFInt32:	return -16; break;
		case FIELDTYPE_MFColor:	return -17; break;
		case FIELDTYPE_MFVec2f:	return -18; break;
		case FIELDTYPE_MFVec3f:	return -19; break;
		case FIELDTYPE_MFVec3d: return -20; break;
		case FIELDTYPE_SFDouble: return sizeof (double); break;
		case FIELDTYPE_MFDouble: return -21; break;
		case FIELDTYPE_SFVec4d: return sizeof (struct SFVec4d); break;

                default:       return type;
	}
} 



/* how many numbers/etc in an array entry? eg, SFVec3f = 3 - 3 floats */
/*		"" ""			eg, MFVec3f = 3 - 3 floats, too! */
int returnElementRowSize (int type) {
	switch (type) {
		case FIELDTYPE_SFVec2f:
		case FIELDTYPE_MFVec2f:
			return 2;
		case FIELDTYPE_SFColor:
		case FIELDTYPE_MFColor:
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFVec3d:
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_SFImage: /* initialization - we can have a "0,0,0" for no texture */
			return 3;
		case FIELDTYPE_SFRotation:
		case FIELDTYPE_MFRotation:
		case FIELDTYPE_SFVec4d:
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_MFColorRGBA:
			return 4;
	}
	return 1;

}

static struct VRMLParser *parser = NULL;
void Parser_scanStringValueToMem(struct X3D_Node *node, int coffset, int ctype, char *value, int isXML) {
	char *nst;                      /* used for pointer maths */
	union anyVrml myVal;
	char *mfstringtmp = NULL;
	int oldXMLflag;
	struct X3D_Node *np;
	
	#ifdef SETFIELDVERBOSE
	printf ("\nPST, for %s we have %s strlen %d\n",stringFieldtypeType(ctype), value, strlen(value));
	#endif

	/* if this is the first time through, create a new parser, and tell it:
	      - that we are using X3D formatted field strings, NOT "VRML" ones;
	      - that the destination node is not important (the NULL, offset 0) */

	if (parser == NULL) parser=newParser(NULL, 0, TRUE);

	/* there is a difference sometimes, in the XML format and VRML classic format. The XML
	   parser will use xml format, scripts and EAI will use the classic format */
	oldXMLflag = parser->parsingX3DfromXML;
	parser->parsingX3DfromXML = isXML;

	/* we NEED MFStrings to have quotes on; so if this is a MFString, ensure quotes are ok */
	if (ctype == FIELDTYPE_MFString) {
		#ifdef SETFIELDVERBOSE
		printf ("parsing type %s, string :%s:\n",stringFieldtypeType(ctype),value); 
		#endif

		/* go to the first non-space character, and see if this is required;
		   sometimes people will encode mfstrings as:
			url=' "images/earth.gif" "http://ww
		   note the space in the value */
		while ((*value == ' ') && (*value != '\0')) value ++;

		/* now, does the value string need quoting? */
		if ((*value != '"') && (*value != '\'') && (*value != '[')) {
			int len;
			/* printf ("have to quote this string\n"); */
			len = strlen(value);
			mfstringtmp = MALLOC (sizeof (char *) * len + 10);
			memcpy (&mfstringtmp[1],value,len);
			mfstringtmp[0] = '"';
			mfstringtmp[len+1] = '"';
			mfstringtmp[len+2] = '\0';
			/* printf ("so, mfstring is :%s:\n",mfstringtmp); */
			
			parser_fromString(parser,mfstringtmp);
		} else {
			parser_fromString(parser,value);
		}
        } else if (ctype == FIELDTYPE_SFNode) {
                /* Need to change index to proper node ptr */
                np = getEAINodeFromTable(atoi(value), -1);
        } else {

		parser_fromString(parser, value);
	}

	ASSERT(parser->lexer);
	FREE_IF_NZ(parser->lexer->curID);

        if (ctype == FIELDTYPE_SFNode) {
                nst = (char *) node;
                nst += coffset;
                struct X3D_Node* oldvalue;
                memcpy (&oldvalue, nst, sizeof(struct X3D_Node*));
                if (oldvalue) {
                        remove_parent(oldvalue, node);
                }
                memcpy((void*)nst, (void*)&np, sizeof(struct X3D_Node*));
                add_parent(np, node, "sarah's add", 0);

        } else if (parseType(parser, ctype, &myVal)) {

		/* printf ("parsed successfully\n");  */

		nst = (char *) node; /* should be 64 bit compatible */
		nst += coffset; 


/*
MF_TYPE(MFNode, mfnode, Node)
*/
		switch (ctype) {

			PST_MF_STRUCT_ELEMENT(Vec2f,vec2f)
			PST_MF_STRUCT_ELEMENT(Vec3f,vec3f)
			PST_MF_STRUCT_ELEMENT(Vec3d,vec3d)
			PST_MF_STRUCT_ELEMENT(Vec4d,vec4d)
			PST_MF_STRUCT_ELEMENT(Vec2d,vec2d)
			PST_MF_STRUCT_ELEMENT(Color,color)
			PST_MF_STRUCT_ELEMENT(ColorRGBA,colorrgba)
			PST_MF_STRUCT_ELEMENT(Int32,int32)
			PST_MF_STRUCT_ELEMENT(Float,float)
			PST_MF_STRUCT_ELEMENT(Double,double)
			PST_MF_STRUCT_ELEMENT(Bool,bool)
			PST_MF_STRUCT_ELEMENT(Time,time)
			PST_MF_STRUCT_ELEMENT(Rotation,rotation)
			PST_MF_STRUCT_ELEMENT(Matrix3f,matrix3f)
			PST_MF_STRUCT_ELEMENT(Matrix3d,matrix3d)
			PST_MF_STRUCT_ELEMENT(Matrix4f,matrix4f)
			PST_MF_STRUCT_ELEMENT(Matrix4d,matrix4d)
			PST_MF_STRUCT_ELEMENT(String,string)

			PST_SF_SIMPLE_ELEMENT(Float,float,sizeof(float))
			PST_SF_SIMPLE_ELEMENT(Time,time,sizeof(double))
			PST_SF_SIMPLE_ELEMENT(Double,double,sizeof(double))
			PST_SF_SIMPLE_ELEMENT(Int32,int32,sizeof(int))
			PST_SF_SIMPLE_ELEMENT(Bool,bool,sizeof(int))
			PST_SF_SIMPLE_ELEMENT(Node,node,sizeof(void *))
			PST_SF_SIMPLE_ELEMENT(Vec2f,vec2f,sizeof(struct SFVec2f))
			PST_SF_SIMPLE_ELEMENT(Vec2d,vec2d,sizeof(struct SFVec2d))
			PST_SF_SIMPLE_ELEMENT(Vec3f,vec3f,sizeof(struct SFColor))
			PST_SF_SIMPLE_ELEMENT(Vec3d,vec3d,sizeof(struct SFVec3d))
			PST_SF_SIMPLE_ELEMENT(Vec4d,vec4d,sizeof(struct SFVec4d))
			PST_SF_SIMPLE_ELEMENT(Rotation,rotation,sizeof(struct SFRotation))
			PST_SF_SIMPLE_ELEMENT(Color,color,sizeof(struct SFColor))
			PST_SF_SIMPLE_ELEMENT(ColorRGBA,colorrgba,sizeof(struct SFColorRGBA))
			PST_SF_SIMPLE_ELEMENT(Matrix3f,matrix3f,sizeof(struct SFMatrix3f))
			PST_SF_SIMPLE_ELEMENT(Matrix4f,matrix4f,sizeof(struct SFMatrix4f))
			PST_SF_SIMPLE_ELEMENT(Matrix3d,matrix3d,sizeof(struct SFMatrix3d))
			PST_SF_SIMPLE_ELEMENT(Matrix4d,matrix4d,sizeof(struct SFMatrix4d))
			PST_SF_SIMPLE_ELEMENT(Image,image,sizeof(struct Multi_Int32))

			case FIELDTYPE_SFString: {
					struct Uni_String *mptr;
					mptr = * (struct Uni_String **)nst;
					FREE_IF_NZ(mptr->strptr);
					mptr->strptr = myVal.sfstring->strptr;
					mptr->len = myVal.sfstring->len;
					mptr->touched = myVal.sfstring->touched;
				break; }

			default: {
				printf ("unhandled type, in EAIParse  %s\n",stringFieldtypeType(ctype));
				return;
			}
		}

	} else {
		if (strlen (value) > 50) {
			value[45] = '.';
			value[46] = '.';
			value[47] = '.';
			value[48] = '\0';
		}
		ConsoleMessage ("parser problem on parsing fieldType %s, string :%s:", stringFieldtypeType(ctype),value);
	}

	/* we tell our little lexer here that it has no input; otherwise next time through, the 
	   parser_fromString call will cause malloc problems */
	parser->lexer->startOfStringPtr = NULL;

	/* did we have to do any mfstring quoting stuff? */
	FREE_IF_NZ(mfstringtmp);

	/* and, reset the XML flag */
	parser->parsingX3DfromXML = oldXMLflag;
}
