/*
=INSERT_TEMPLATE_HERE=

$Id: CParseGeneral.h,v 1.3 2008/12/02 17:41:38 couannette Exp $

General header for VRML-parser (lexer/parser)

*/

#ifndef __FREEX3D_CPARSE_GENERAL_H__
#define __FREEX3D_CPARSE_GENERAL_H__


/* Typedefs for VRML-types. */
typedef int	vrmlBoolT;
typedef struct SFColor	vrmlColorT;
typedef struct SFColorRGBA	vrmlColorRGBAT;
typedef float	vrmlFloatT;
typedef int32_t	vrmlInt32T;
typedef struct Multi_Int32	vrmlImageT;
typedef struct X3D_Node*	vrmlNodeT;
typedef struct SFRotation	vrmlRotationT;
typedef struct Uni_String*	vrmlStringT;
typedef double	vrmlTimeT;
typedef double	vrmlDoubleT;
typedef struct SFVec2f	vrmlVec2fT;
typedef struct SFVec2d	vrmlVec2dT;
typedef struct SFVec4f	vrmlVec4fT;
typedef struct SFVec4d	vrmlVec4dT;
typedef struct SFColor	vrmlVec3fT;
typedef struct SFVec3d  vrmlVec3dT;
typedef struct SFMatrix3f	vrmlMatrix3fT;
typedef struct SFMatrix3d vrmlMatrix3dT;
typedef struct SFMatrix4f	vrmlMatrix4fT;
typedef struct SFMatrix4d vrmlMatrix4dT;

/* This is an union to hold every vrml-type */
union anyVrml
{
 #define SF_TYPE(fttype, type, ttype) \
  vrml##ttype##T type;
 #define MF_TYPE(fttype, type, ttype) \
  struct Multi_##ttype type;
 #include "VrmlTypeList.h"
 #undef SF_TYPE
 #undef MF_TYPE
};

#define parseError(msg) \
 (ConsoleMessage("Parse error:  " msg "\n"), fprintf(stderr, msg "\n")) \

#define CPARSE_ERROR_CURID(str) \
		strcpy (fw_outline,str); \
		strcat (fw_outline,"expected colon in COMPONENT statement, found \""); \
		if (me->lexer->curID != ((void *)0)) strcat (fw_outline, me->lexer->curID); \
		else strcat (fw_outline, "(EOF)"); \
		strcat (fw_outline,"\" "); \
		ConsoleMessage(fw_outline); \
		fprintf (stderr,"%s\n",fw_outline);

/* tie assert in here to give better failure methodology */
/* #define ASSERT(cond) if(!(cond)){fw_assert(__FILE__,__LINE__);} */
/* void fw_assert(char *,int); */


#endif /* __FREEX3D_CPARSE_GENERAL_H__ */
