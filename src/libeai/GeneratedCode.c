

/* GeneratedCode.c  generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */
/******************************************************************* 
Copyright (C) 2007 John Stewart (CRC Canada) 
DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED. 
See the GNU Library General Public License (file COPYING in the distribution) 
for conditions of use and redistribution. 
*********************************************************************/ 



/* GENERATED BY VRMLC.pm - do NOT change this file */

#include "EAI_C.h"
/* convert an internal type to EAI type */
char mapFieldTypeToEAItype (int st) {
	switch (st) { 
		case FIELDTYPE_SFFloat:	return EAI_SFFloat;
		case FIELDTYPE_MFFloat:	return EAI_MFFloat;
		case FIELDTYPE_SFRotation:	return EAI_SFRotation;
		case FIELDTYPE_MFRotation:	return EAI_MFRotation;
		case FIELDTYPE_SFVec3f:	return EAI_SFVec3f;
		case FIELDTYPE_MFVec3f:	return EAI_MFVec3f;
		case FIELDTYPE_SFBool:	return EAI_SFBool;
		case FIELDTYPE_MFBool:	return EAI_MFBool;
		case FIELDTYPE_SFInt32:	return EAI_SFInt32;
		case FIELDTYPE_MFInt32:	return EAI_MFInt32;
		case FIELDTYPE_SFNode:	return EAI_SFNode;
		case FIELDTYPE_MFNode:	return EAI_MFNode;
		case FIELDTYPE_SFColor:	return EAI_SFColor;
		case FIELDTYPE_MFColor:	return EAI_MFColor;
		case FIELDTYPE_SFColorRGBA:	return EAI_SFColorRGBA;
		case FIELDTYPE_MFColorRGBA:	return EAI_MFColorRGBA;
		case FIELDTYPE_SFTime:	return EAI_SFTime;
		case FIELDTYPE_MFTime:	return EAI_MFTime;
		case FIELDTYPE_SFString:	return EAI_SFString;
		case FIELDTYPE_MFString:	return EAI_MFString;
		case FIELDTYPE_SFVec2f:	return EAI_SFVec2f;
		case FIELDTYPE_MFVec2f:	return EAI_MFVec2f;
		case FIELDTYPE_SFImage:	return EAI_SFImage;
		case FIELDTYPE_FreeWRLPTR:	return EAI_FreeWRLPTR;
		case FIELDTYPE_SFVec3d:	return EAI_SFVec3d;
		case FIELDTYPE_MFVec3d:	return EAI_MFVec3d;
		case FIELDTYPE_SFDouble:	return EAI_SFDouble;
		case FIELDTYPE_MFDouble:	return EAI_MFDouble;
		case FIELDTYPE_SFMatrix3f:	return EAI_SFMatrix3f;
		case FIELDTYPE_MFMatrix3f:	return EAI_MFMatrix3f;
		case FIELDTYPE_SFMatrix3d:	return EAI_SFMatrix3d;
		case FIELDTYPE_MFMatrix3d:	return EAI_MFMatrix3d;
		case FIELDTYPE_SFMatrix4f:	return EAI_SFMatrix4f;
		case FIELDTYPE_MFMatrix4f:	return EAI_MFMatrix4f;
		case FIELDTYPE_SFMatrix4d:	return EAI_SFMatrix4d;
		case FIELDTYPE_MFMatrix4d:	return EAI_MFMatrix4d;
		case FIELDTYPE_SFVec2d:	return EAI_SFVec2d;
		case FIELDTYPE_MFVec2d:	return EAI_MFVec2d;
		case FIELDTYPE_SFVec4f:	return EAI_SFVec4f;
		case FIELDTYPE_MFVec4f:	return EAI_MFVec4f;
		case FIELDTYPE_SFVec4d:	return EAI_SFVec4d;
		case FIELDTYPE_MFVec4d:	return EAI_MFVec4d;
		default: return -1;
	}
	return -1;
}
/* convert an EAI type to an internal type */
int mapEAItypeToFieldType (char st) {
	switch (st) { 
		case EAI_SFFloat:	return FIELDTYPE_SFFloat;
		case EAI_MFFloat:	return FIELDTYPE_MFFloat;
		case EAI_SFRotation:	return FIELDTYPE_SFRotation;
		case EAI_MFRotation:	return FIELDTYPE_MFRotation;
		case EAI_SFVec3f:	return FIELDTYPE_SFVec3f;
		case EAI_MFVec3f:	return FIELDTYPE_MFVec3f;
		case EAI_SFBool:	return FIELDTYPE_SFBool;
		case EAI_MFBool:	return FIELDTYPE_MFBool;
		case EAI_SFInt32:	return FIELDTYPE_SFInt32;
		case EAI_MFInt32:	return FIELDTYPE_MFInt32;
		case EAI_SFNode:	return FIELDTYPE_SFNode;
		case EAI_MFNode:	return FIELDTYPE_MFNode;
		case EAI_SFColor:	return FIELDTYPE_SFColor;
		case EAI_MFColor:	return FIELDTYPE_MFColor;
		case EAI_SFColorRGBA:	return FIELDTYPE_SFColorRGBA;
		case EAI_MFColorRGBA:	return FIELDTYPE_MFColorRGBA;
		case EAI_SFTime:	return FIELDTYPE_SFTime;
		case EAI_MFTime:	return FIELDTYPE_MFTime;
		case EAI_SFString:	return FIELDTYPE_SFString;
		case EAI_MFString:	return FIELDTYPE_MFString;
		case EAI_SFVec2f:	return FIELDTYPE_SFVec2f;
		case EAI_MFVec2f:	return FIELDTYPE_MFVec2f;
		case EAI_SFImage:	return FIELDTYPE_SFImage;
		case EAI_FreeWRLPTR:	return FIELDTYPE_FreeWRLPTR;
		case EAI_SFVec3d:	return FIELDTYPE_SFVec3d;
		case EAI_MFVec3d:	return FIELDTYPE_MFVec3d;
		case EAI_SFDouble:	return FIELDTYPE_SFDouble;
		case EAI_MFDouble:	return FIELDTYPE_MFDouble;
		case EAI_SFMatrix3f:	return FIELDTYPE_SFMatrix3f;
		case EAI_MFMatrix3f:	return FIELDTYPE_MFMatrix3f;
		case EAI_SFMatrix3d:	return FIELDTYPE_SFMatrix3d;
		case EAI_MFMatrix3d:	return FIELDTYPE_MFMatrix3d;
		case EAI_SFMatrix4f:	return FIELDTYPE_SFMatrix4f;
		case EAI_MFMatrix4f:	return FIELDTYPE_MFMatrix4f;
		case EAI_SFMatrix4d:	return FIELDTYPE_SFMatrix4d;
		case EAI_MFMatrix4d:	return FIELDTYPE_MFMatrix4d;
		case EAI_SFVec2d:	return FIELDTYPE_SFVec2d;
		case EAI_MFVec2d:	return FIELDTYPE_MFVec2d;
		case EAI_SFVec4f:	return FIELDTYPE_SFVec4f;
		case EAI_MFVec4f:	return FIELDTYPE_MFVec4f;
		case EAI_SFVec4d:	return FIELDTYPE_SFVec4d;
		case EAI_MFVec4d:	return FIELDTYPE_MFVec4d;
		default: return -1;
	}
	return -1;
}

/* Table of Field Types */
       const char *FIELDTYPES[] = {
	"SFFloat",
	"MFFloat",
	"SFRotation",
	"MFRotation",
	"SFVec3f",
	"MFVec3f",
	"SFBool",
	"MFBool",
	"SFInt32",
	"MFInt32",
	"SFNode",
	"MFNode",
	"SFColor",
	"MFColor",
	"SFColorRGBA",
	"MFColorRGBA",
	"SFTime",
	"MFTime",
	"SFString",
	"MFString",
	"SFVec2f",
	"MFVec2f",
	"SFImage",
	"FreeWRLPTR",
	"SFVec3d",
	"MFVec3d",
	"SFDouble",
	"MFDouble",
	"SFMatrix3f",
	"MFMatrix3f",
	"SFMatrix3d",
	"MFMatrix3d",
	"SFMatrix4f",
	"MFMatrix4f",
	"SFMatrix4d",
	"MFMatrix4d",
	"SFVec2d",
	"MFVec2d",
	"SFVec4f",
	"MFVec4f",
	"SFVec4d",
	"MFVec4d",
};
const indexT FIELDTYPES_COUNT = ARR_SIZE(FIELDTYPES);
