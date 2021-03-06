/*
=INSERT_TEMPLATE_HERE=

$Id: jsUtils.c,v 1.44 2012/04/14 22:46:32 dug9 Exp $

A substantial amount of code has been adapted from js/src/js.c,
which is the sample application included with the javascript engine.

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
#include "../vrml_parser/CParseGeneral.h"
#include "../main/Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/EAIHelpers.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"

#include "JScript.h"
#include "CScripts.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"
#include "fieldSet.h"
#ifdef _MSC_VER
#include <pthread.h> // win32 needs the strtok_r 
#endif

#ifdef HAVE_JAVASCRIPT

#ifdef WANT_OSC
	#include "../scenegraph/ringbuf.h"
	#define USE_OSC 1
	#define TRACK_FIFO_MSG 0
#else
	#define USE_OSC 0
	#define TRACK_FIFO_MSG 0
#endif
#define TRY_PROTO_FIX 1
extern void dump_scene (FILE *fp, int level, struct X3D_Node* node); // in GeneratedCode.c
extern char *parser_getNameFromNode(struct X3D_Node *ptr) ; /* vi +/dump_scene src/lib/scenegraph/GeneratedCode.c */

/********************** Javascript to X3D Scenegraph ****************************/


/* a SF value has changed in an MF; eg, xx[10] = new SFVec3f(...); */
/* These values were created below; new SF values to this MF are not accessed here, but in 
   the MFVec3fSetProperty (or equivalent). Note that we do NOT use something like JS_SetElement
   on these because it would be a recursive call; thus we set the private data */


//static int insetSFStr = FALSE;
//static JSBool reportWarnings = JS_TRUE;
typedef struct pjsUtils{
	int insetSFStr;// = FALSE;
	JSBool reportWarnings;// = JS_TRUE;

}* ppjsUtils;
void *jsUtils_constructor(){
	void *v = malloc(sizeof(struct pjsUtils));
	memset(v,0,sizeof(struct pjsUtils));
	return v;
}
void jsUtils_init(struct tjsUtils *t){
	//public
	//private
	t->prv = jsUtils_constructor();
	{
		ppjsUtils p = (ppjsUtils)t->prv;
		p->insetSFStr = FALSE;
		p->reportWarnings = JS_TRUE;

	}
}
//	ppjsUtils p = (ppjsUtils)gglobal()->jsUtils.prv;
#if JS_VERSION < 185
static JSBool setSF_in_MF (JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
#else
static JSBool setSF_in_MF (JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp) {
#endif
	int num;
	jsval pf;
	jsval nf;
	JSObject *me;
	JSObject *par;
	jsval ele; 
	ppjsUtils p = (ppjsUtils)gglobal()->jsUtils.prv;
	jsid oid;
#if JS_VERSION >= 185
	char *tmp;
	jsval id;
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("setSF_in_MF: JS_IdToValue failed.\n");
		return JS_FALSE;
	}
#endif

	/* when we save the value, we will be called again, so we make sure that we
	   know if we are being called from within, or from without */
	if (p->insetSFStr) { 
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("setSF_in_MF: already caught this value; this is our JS_SetElement call\n"); 
		#endif
		return JS_TRUE;
	}

	/* ok, we are really called to replace an existing SFNode MF value assignment */
	p->insetSFStr = TRUE; 

	if (JSVAL_IS_INT(id)) {
		if (!JS_ValueToInt32(cx,id,&num)) {
			printf ("setSF_in_MF: error converting to number...\n");
			return JS_FALSE;
		}
		/* get a pointer to the object at the index in the parent */ 
		if (!JS_GetElement(cx, obj, num, &ele)) { 
			printf ("error getting child %d in setSF_in_MF\n",num); 
			return JS_FALSE; 
		} 
		/* THIS is the touching that will cause us to be called recursively,
		   which is why insetSFStr is TRUE right here */
		if (!JS_SetElement(cx,obj,num,vp)) { 
			printf ("can not set element %d in MFString\n",num); 
			return JS_FALSE; 
		} 
	} else {
		printf ("expect an integer id in setSF_in_MF\n");
		return JS_FALSE;
	}


	/* copy this value out to the X3D scene graph */
	me = obj;
	par = JS_GetParent(cx, me);
	while (par != NULL) {
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("for obj %u: ",me);
			printJSNodeType(cx,me);
		printf ("... parent %u\n",par);
			printJSNodeType(cx,par);
		#endif

		if (JS_InstanceOf (cx, par, &SFNodeClass, NULL)) {
			#ifdef JSVRMLCLASSESVERBOSE
			printf (" the parent IS AN SFNODE - it is %u\n",par);
			#endif


			if (!JS_GetProperty(cx, obj, "_parentField", &pf)) {
				printf ("doMFSetProperty, can not get parent field from this object\n");
				return JS_FALSE;
			}

			nf = OBJECT_TO_JSVAL(me);

			#ifdef JSVRMLCLASSESVERBOSE
			#if JS_VERSION < 185
			printf ("parentField is %u \"%s\"\n", pf, JS_GetStringBytes(JSVAL_TO_STRING(pf)));
			#else
			tmp=JS_EncodeString(cx,JSVAL_TO_STRING(pf));
			printf ("parentField is %u \"%s\"\n", pf, tmp);
			JS_free(cx,tmp);
			#endif
			#endif

			if (!JS_ValueToId(cx,pf,&oid)) {
				printf("setSF_in_MF: JS_ValueToId failed.\n");
				return JS_FALSE;
			}

			if (!setSFNodeField (cx, par, oid,
#if JS_VERSION >= 185
			   JS_FALSE,
#endif
			   &nf)) {
				printf ("could not set field of SFNode\n");
			}

		}
		me = par;
		par = JS_GetParent(cx, me);
	}
	p->insetSFStr = FALSE;
	return JS_TRUE;
}

/* take an ECMA value in the X3D Scenegraph, and return a jsval with it in */
/* This is FAST as w deal just with pointers */
static void JS_ECMA_TO_X3D(JSContext *cx, void *Data, unsigned datalen, int dataType, jsval *newval) {
	float fl;
	double dl;
	int il;

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("calling JS_ECMA_TO_X3D on type %s\n",FIELDTYPES[dataType]);
	#endif

	switch (dataType) {

		case FIELDTYPE_SFFloat:	{
			if (!JS_ValueToNumber(cx,*newval,&dl)) {
				printf ("problems converting Javascript val to number\n");
				return;
			}
			fl = (float) dl;
			memcpy (Data, (void *) &fl, datalen);
			break;
		}
		case FIELDTYPE_SFDouble:
		case FIELDTYPE_SFTime:	{
			if (!JS_ValueToNumber(cx,*newval,&dl)) {
				printf ("problems converting Javascript val to number\n");
				return;
			}
			memcpy (Data, (void *) &dl, datalen);
			break;
		}
		case FIELDTYPE_SFBool: {
			il = JSVAL_TO_BOOLEAN (*newval);
			memcpy (Data, (void *) &il, datalen);
			break;
		}

		case FIELDTYPE_SFInt32: 	{ 
			il = JSVAL_TO_INT (*newval);
			memcpy (Data, (void *) &il, datalen);
			break;
		}

		case FIELDTYPE_SFString: {
			struct Uni_String *oldS;
        		JSString *_idStr;
        		char *_id_c;

        		_idStr = JS_ValueToString(cx, *newval);
#if JS_VERSION < 185
        		_id_c = JS_GetStringBytes(_idStr);
#else
        		_id_c = JS_EncodeString(cx,_idStr);
#endif

			oldS = (struct Uni_String *) *((uintptr_t *)Data);

			#ifdef JSVRMLCLASSESVERBOSE
			printf ("JS_ECMA_TO_X3D, replacing \"%s\" with \"%s\" \n", oldS->strptr, _id_c);
			#endif

			/* replace the C string if it needs to be replaced. */
			verify_Uni_String (oldS,_id_c);
#if JS_VERSION >= 185
			JS_free(cx,_id_c);
#endif
			break;
		}
		default: {	printf("WARNING: SHOULD NOT BE HERE in JS_ECMA_TO_X3D! %d\n",dataType); }
	}
}


/* take a Javascript  ECMA value and put it in the X3D Scenegraph. */
static void JS_SF_TO_X3D(JSContext *cx, void *Data, unsigned datalen, int dataType, jsval *newval) {
        SFColorNative *Cptr;
	SFVec3fNative *V3ptr;
	SFVec3dNative *V3dptr;
	SFVec2fNative *V2ptr;
	SFRotationNative *VRptr;
	SFNodeNative *VNptr;

	void *VPtr;

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("calling JS_SF_TO_X3D on type %s\n",FIELDTYPES[dataType]);
	#endif

	/* get a pointer to the internal private data */
	if ((VPtr = JS_GetPrivate(cx, JSVAL_TO_OBJECT(*newval))) == NULL) {
		printf( "JS_GetPrivate failed in JS_SF_TO_X3D.\n");
		return;
	}

	/* copy over the data from the X3D node to this new Javascript object */
	switch (dataType) {
                case FIELDTYPE_SFColor:
			Cptr = (SFColorNative *)VPtr;
			memcpy (Data, (void *)((Cptr->v).c), datalen);
			break;
                case FIELDTYPE_SFVec3d:
			V3dptr = (SFVec3dNative *)VPtr;
			memcpy (Data, (void *)((V3dptr->v).c), datalen);
			break;
                case FIELDTYPE_SFVec3f:
			V3ptr = (SFVec3fNative *)VPtr;
			memcpy (Data, (void *)((V3ptr->v).c), datalen);
			break;
                case FIELDTYPE_SFVec2f:
			V2ptr = (SFVec2fNative *)VPtr;
			memcpy (Data, (void *)((V2ptr->v).c), datalen);
			break;
                case FIELDTYPE_SFRotation:
			VRptr = (SFRotationNative *)VPtr;
			memcpy (Data,(void *)((VRptr->v).c), datalen);
			break;
                case FIELDTYPE_SFNode:
			VNptr = (SFNodeNative *)VPtr;
			memcpy (Data, (void *)(VNptr->handle), datalen);
			break;

		default: {	printf("WARNING: SHOULD NOT BE HERE! %d\n",dataType); }
	}
}


/* make an MF type from the X3D node. This can be fairly slow... */
static void JS_MF_TO_X3D(JSContext *cx, JSObject * obj, void *Data, int dataType, jsval *newval) {
	ttglobal tg = gglobal();
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("calling JS_MF_TO_X3D on type %s\n",FIELDTYPES[dataType]);
	printf ("JS_MF_TO_X3D, we have object %u, newval %u\n",obj,*newval);
	printf ("JS_MF_TO_X3D, obj is an:\n");
	if (JSVAL_IS_OBJECT(OBJECT_TO_JSVAL(obj))) { printf ("JS_MF_TO_X3D - obj is an OBJECT\n"); }
	if (JSVAL_IS_PRIMITIVE(OBJECT_TO_JSVAL(obj))) { printf ("JS_MF_TO_X3D - obj is an PRIMITIVE\n"); }
	printf ("JS_MF_TO_X3D, obj is a "); printJSNodeType(cx,obj);
	printf ("JS_MF_TO_X3D, vp is an:\n");
	if (JSVAL_IS_OBJECT(*newval)) { printf ("JS_MF_TO_X3D - vp is an OBJECT\n"); }
	if (JSVAL_IS_PRIMITIVE(*newval)) { printf ("JS_MF_TO_X3D - vp is an PRIMITIVE\n"); }
	printf ("JS_MF_TO_X3D, vp is a "); printJSNodeType(cx,JSVAL_TO_OBJECT(*newval));
	#endif

	tg->CRoutes.JSglobal_return_val = *newval;
	getJSMultiNumType (cx, (struct Multi_Vec3f*) Data, convertToSFType(dataType));

}

/********************** X3D Scenegraph to Javascript ****************************/

/* take an ECMA value in the X3D Scenegraph, and return a jsval with it in */
/* This is FAST as w deal just with pointers */
void X3D_ECMA_TO_JS(JSContext *cx, void *Data, int datalen, int dataType, jsval *newval) {
	float fl;
	double dl;
	int il;

	/* NOTE - caller of this function has already defined a BeginRequest */

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("calling X3D_ECMA_TO_JS on type %s\n",FIELDTYPES[dataType]);
	#endif

	switch (dataType) {

		case FIELDTYPE_SFFloat:	{
			memcpy ((void *) &fl, Data, datalen);
			JS_NewNumberValue(cx,(double)fl,newval);
			break;
		}
		case FIELDTYPE_SFDouble:
		case FIELDTYPE_SFTime:	{
			memcpy ((void *) &dl, Data, datalen);
			JS_NewNumberValue(cx,dl,newval);
			break;
		}
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFInt32: 	{ 
			memcpy ((void *) &il,Data, datalen);
			*newval = INT_TO_JSVAL(il);
			break;
		}

		case FIELDTYPE_SFString: {
			struct Uni_String *ms;

			/* datalen will be ROUTING_SFSTRING here; or at least should be! We
			   copy over the data, which is a UniString pointer, and use the pointer
			   value here */
			memcpy((void *) &ms,Data, sizeof(void *));
			*newval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,ms->strptr));
			break;
		}
		default: {	printf("WARNING: SHOULD NOT BE HERE in X3D_ECMA_TO_JS! %d\n",dataType); }
	}
}

/* take an ECMA value in the X3D Scenegraph, and return a jsval with it in */
/* this is not so fast; we call a script to make a default type, then we fill it in */
static void X3D_SF_TO_JS(JSContext *cx, JSObject *obj, void *Data, unsigned datalen, int dataType, jsval *newval) {
        SFColorNative *Cptr;
	SFVec3fNative *V3ptr;
	SFVec3dNative *V3dptr;
	SFVec2fNative *V2ptr;
	SFRotationNative *VRptr;
	SFNodeNative *VNptr;

	void *VPtr;
	jsval rval;
	char *script = NULL;

	/* NOTE - caller is (eventually) a class constructor, no need to BeginRequest */

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("calling X3D_SF_TO_JS on type %s, newval %u\n",FIELDTYPES[dataType],*newval);
	#endif

	if (!JSVAL_IS_OBJECT(*newval)) {
		/* find a script to create the correct object */
		switch (dataType) {
        	        case FIELDTYPE_SFVec3f: script = "new SFVec3f()"; break;
        	        case FIELDTYPE_SFVec3d: script = "new SFVec3d()"; break;
        	        case FIELDTYPE_SFColor: script = "new SFColor()"; break;
        	        case FIELDTYPE_SFNode: script = "new SFNode()"; break;
        	        case FIELDTYPE_SFVec2f: script = "new SFVec2f()"; break;
        	        case FIELDTYPE_SFRotation: script = "new SFRotation()"; break;
			default: printf ("invalid type in X3D_SF_TO_JS\n"); return;
		}

		/* create the object */


		#ifdef JSVRMLCLASSESVERBOSE
		printf ("X3D_SF_TO_JS, have to run script to make new object: \"%s\"\n",script);
		#endif

		if (!JS_EvaluateScript(cx, obj, script, (int) strlen(script), FNAME_STUB, LINENO_STUB, &rval)) {
			printf ("error creating the new object in X3D_SF_TO_JS, script :%s:\n",script);
			return;
		}

		/* this is the return pointer, lets save it right now */
		*newval = rval;

		#ifdef JSVRMLCLASSESVERBOSE
		printf ("X3D_SF_TO_JS, so, newval now is %u\n",*newval);
		#endif

	}
	/* get a pointer to the internal private data */
	if ((VPtr = JS_GetPrivate(cx, JSVAL_TO_OBJECT(*newval))) == NULL) {
		printf( "JS_GetPrivate failed in X3D_SF_TO_JS.\n");
		return;
	}


	/* copy over the data from the X3D node to this new Javascript object */
	switch (dataType) {
                case FIELDTYPE_SFColor:
			Cptr = (SFColorNative *)VPtr;
			memcpy ((void *)((Cptr->v).c), Data, datalen);
        		Cptr->valueChanged = 1;
			break;
                case FIELDTYPE_SFVec3f:
			V3ptr = (SFVec3fNative *)VPtr;
			memcpy ((void *)((V3ptr->v).c), Data, datalen);
        		V3ptr->valueChanged = 1;
			break;
                case FIELDTYPE_SFVec3d:
			V3dptr = (SFVec3dNative *)VPtr;
			memcpy ((void *)((V3dptr->v).c), Data, datalen);
        		V3dptr->valueChanged = 1;
			break;
                case FIELDTYPE_SFVec2f:
			V2ptr = (SFVec2fNative *)VPtr;
			memcpy ((void *)((V2ptr->v).c), Data, datalen);
        		V2ptr->valueChanged = 1;
			break;
                case FIELDTYPE_SFRotation:
			VRptr = (SFRotationNative *)VPtr;
			memcpy ((void *)((VRptr->v).c), Data, datalen);
        		VRptr->valueChanged = 1;
			break;
                case FIELDTYPE_SFNode:
			VNptr = (SFNodeNative *)VPtr;
			memcpy ((void *)(&(VNptr->handle)), Data, datalen);
        		VNptr->valueChanged = 1;
			break;

		default: {	printf("WARNING: SHOULD NOT BE HERE! %d\n",dataType); }
	}
}

/* make an MF type from the X3D node. This can be fairly slow... */
static void X3D_MF_TO_JS(JSContext *cx, JSObject *obj, void *Data, int dataType, jsval *newval, char *fieldName) {
	int i;
	jsval rval;
	char *script = NULL;
	struct Multi_Int32 *MIptr;
	struct Multi_Float *MFptr;
	struct Multi_Time *MTptr;
	jsval fieldNameAsJSVAL;

	/* NOTE - caller is (eventually) a JS class constructor, no need to BeginRequest */

	/* so, obj should be an __SFNode_proto, and newval should be a __MFString_proto (or whatever) */

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("calling X3D_MF_TO_JS on type %s\n",FIELDTYPES[dataType]);
	printf ("X3D_MF_TO_JS, we have object %u, newval %u\n",obj,*newval);
	printf ("X3D_MF_TO_JS, obj is an:\n");
	if (JSVAL_IS_OBJECT(OBJECT_TO_JSVAL(obj))) { printf ("X3D_MF_TO_JS - obj is an OBJECT\n"); 
		printf ("X3D_MF_TO_JS, obj is a "); printJSNodeType(cx,obj);
	}
	if (JSVAL_IS_PRIMITIVE(OBJECT_TO_JSVAL(obj))) { printf ("X3D_MF_TO_JS - obj is an PRIMITIVE\n"); }
	printf ("X3D_MF_TO_JS, vp is an:\n");
	if (JSVAL_IS_OBJECT(*newval)) { printf ("X3D_MF_TO_JS - newval is an OBJECT\n"); 
		printf ("X3D_MF_TO_JS, newval is a "); printJSNodeType(cx,JSVAL_TO_OBJECT(*newval));
	}
	if (JSVAL_IS_PRIMITIVE(*newval)) { printf ("X3D_MF_TO_JS - newval is an PRIMITIVE\n"); }
	#endif


#ifdef JSVRMLCLASSESVERBOSE
	printf ("X3D_MF_TO_JS - is this already expanded? \n");
	{
        SFNodeNative *VNptr;

        /* get a pointer to the internal private data */
        if ((VNptr = JS_GetPrivate(cx, obj)) == NULL) {
                printf( "JS_GetPrivate failed in X3D_MF_TO_JS.\n");
                return;
        }
	if (VNptr->fieldsExpanded) printf ("FIELDS EXPANDED\n");
	else printf ("FIELDS NOT EXPANDED\n");
	}
#endif


	if (!JSVAL_IS_OBJECT(*newval)) {
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("X3D_MF_TO_JS - have to create empty MF type \n");
		#endif

		/* find a script to create the correct object */
		switch (dataType) {
			case FIELDTYPE_MFString: script = "new MFString()"; break;
			case FIELDTYPE_MFFloat: script = "new MFFloat()"; break;
			case FIELDTYPE_MFTime: script = "new MFTime()"; break;
			case FIELDTYPE_MFInt32: script = "new MFInt32()"; break;
			case FIELDTYPE_SFImage: script = "new SFImage()"; break;
        	        case FIELDTYPE_MFVec3f: script = "new MFVec3f()"; break;
        	        case FIELDTYPE_MFColor: script = "new MFColor()"; break;
        	        case FIELDTYPE_MFNode: script = "new MFNode()"; break;
        	        case FIELDTYPE_MFVec2f: script = "new MFVec2f()"; break;
        	        case FIELDTYPE_MFRotation: script = "new MFRotation()"; break;
			default: printf ("invalid type in X3D_MF_TO_JS\n"); return;
		}

		if (!JS_EvaluateScript(cx, obj, script, (int) strlen(script), FNAME_STUB, LINENO_STUB, &rval)) {
			printf ("error creating the new object in X3D_MF_TO_JS\n");
			return;
		}

		/* this is the return pointer, lets save it right now */
		*newval = rval;
	}

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("setting parent for %u to %u\n", *newval, obj);
	#endif

	/* ok - if we are setting an MF* field by a thing like myField[10] = new String(); the
	   set method does not really get called. So, we go up the parental chain until we get
	   either no parent, or a SFNode. If we get a SFNode, we call the "save this" function
	   so that the X3D scene graph gets the updated array value. To make a long story short,
	   here's the call to set the parent for the above. */
	if (!JS_SetParent (cx, JSVAL_TO_OBJECT(*newval), obj)) {
		printf ("X3D_MF_TO_JS - can not set parent!\n");
	} 

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("telling %u that it is a child \"%s\" of parent %u\n",*newval, fieldName, obj);
	#endif

	fieldNameAsJSVAL = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,fieldName));

        if (!JS_DefineProperty(cx, JSVAL_TO_OBJECT(*newval), "_parentField", fieldNameAsJSVAL, 
			JS_GET_PROPERTY_STUB, JS_SET_PROPERTY_STUB5, JSPROP_READONLY)) {
                printf("JS_DefineProperty failed for \"%s\" in X3D_MF_TO_JS.\n", fieldName);
                return;
       	}


	#ifdef JSVRMLCLASSESVERBOSE
	printf ("X3D_MF_TO_JS - object is %u, copying over data\n",*newval);
	#endif


	/* copy over the data from the X3D node to this new Javascript object */
	switch (dataType) {
		case FIELDTYPE_MFInt32:
			MIptr = (struct Multi_Int32*) Data;
			for (i=0; i<MIptr->n; i++) {
                		if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(*newval), (jsint) i, INT_TO_JSVAL(MIptr->p[i]),
                        		  JS_GET_PROPERTY_STUB, setSF_in_MF, JSPROP_ENUMERATE)) {
                        		printf( "JS_DefineElement failed for arg %u in MFInt32Constr.\n", i);
                        		return;
                		}
			}
			break;
		case FIELDTYPE_MFFloat:
			MFptr = (struct Multi_Float*) Data;
			for (i=0; i<MFptr->n; i++) {
                		if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(*newval), (jsint) i, INT_TO_JSVAL(MFptr->p[i]),
                        		  JS_GET_PROPERTY_STUB, setSF_in_MF, JSPROP_ENUMERATE)) {
                        		printf( "JS_DefineElement failed for arg %u in MFFloatConstr.\n", i);
                        		return;
                		}
			}
			break;
		case FIELDTYPE_MFTime:
			MTptr = (struct Multi_Time*) Data;
			for (i=0; i<MTptr->n; i++) {
                		if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(*newval), (jsint) i, INT_TO_JSVAL(MTptr->p[i]),
                        		  JS_GET_PROPERTY_STUB, setSF_in_MF, JSPROP_ENUMERATE)) {
                        		printf( "JS_DefineElement failed for arg %u in MFTimeConstr.\n", i);
                        		return;
                		}
			}
			break;
                case FIELDTYPE_MFColor:
		case FIELDTYPE_MFVec3f: {
			struct Multi_Vec3f* MCptr;
			char newline[100];
			jsval xf;

			MCptr = (struct Multi_Vec3f *) Data;
			for (i=0; i<MCptr->n; i++) {
				if (dataType == FIELDTYPE_MFColor) 
					sprintf (newline,"new SFColor(%f, %f, %f)", MCptr->p[i].c[0], MCptr->p[i].c[1], MCptr->p[i].c[2]);	
				else
					sprintf (newline,"new SFColor(%f, %f, %f)", MCptr->p[i].c[0], MCptr->p[i].c[1], MCptr->p[i].c[2]);	
				if (!JS_EvaluateScript(cx, JSVAL_TO_OBJECT(*newval), newline, (int) strlen(newline), FNAME_STUB, LINENO_STUB, &xf)) {
					printf ("error creating the new object in X3D_MF_TO_JS string :%s:\n",newline);
					return;
				}
                		if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(*newval), (jsint) i, xf,
                        		  JS_GET_PROPERTY_STUB, setSF_in_MF, JSPROP_ENUMERATE)) {
                        		printf( "JS_DefineElement failed for arg %u .\n", i);
                        		return;
                		}
			}
		} break;

		case FIELDTYPE_MFVec2f: {
			struct Multi_Vec2f* MCptr;
			char newline[100];
			jsval xf;

			MCptr = (struct Multi_Vec2f *) Data;
			for (i=0; i<MCptr->n; i++) {
				sprintf (newline,"new SFVec2f(%f, %f)", MCptr->p[i].c[0], MCptr->p[i].c[1]);	
				if (!JS_EvaluateScript(cx, JSVAL_TO_OBJECT(*newval), newline, (int) strlen(newline), FNAME_STUB, LINENO_STUB, &xf)) {
					printf ("error creating the new object in X3D_MF_TO_JS string :%s:\n",newline);
					return;
				}
                		if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(*newval), (jsint) i, xf,
                        		  JS_GET_PROPERTY_STUB, setSF_in_MF, JSPROP_ENUMERATE)) {
                        		printf( "JS_DefineElement failed for arg %u .\n", i);
                        		return;
                		}
			}
		} break;
		case FIELDTYPE_MFRotation: {
			struct Multi_Rotation* MCptr;
			char newline[100];
			jsval xf;

			MCptr = (struct Multi_Rotation*) Data;
			for (i=0; i<MCptr->n; i++) {
				sprintf (newline,"new SFRotation(%f, %f, %f, %f)", MCptr->p[i].c[0], MCptr->p[i].c[1], MCptr->p[i].c[2], MCptr->p[i].c[3]);	
				if (!JS_EvaluateScript(cx, JSVAL_TO_OBJECT(*newval), newline, (int) strlen(newline), FNAME_STUB, LINENO_STUB, &xf)) {
					printf ("error creating the new object in X3D_MF_TO_JS string :%s:\n",newline);
					return;
				}
                		if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(*newval), (jsint) i, xf,
                        		  JS_GET_PROPERTY_STUB, setSF_in_MF, JSPROP_ENUMERATE)) {
                        		printf( "JS_DefineElement failed for arg %u .\n", i);
                        		return;
                		}
			}
		} break;

		case FIELDTYPE_MFNode: {
			struct Multi_Node* MCptr;
			char newline[100];
			jsval xf;

			MCptr = (struct Multi_Node *) Data;

			for (i=0; i<MCptr->n; i++) {
				/* purge out null nodes */
				if (MCptr->p[i] != NULL) {
				sprintf (newline,"new SFNode(\"%p\")", MCptr->p[i]);	

				if (!JS_EvaluateScript(cx, JSVAL_TO_OBJECT(*newval), newline, (int) strlen(newline), FNAME_STUB, LINENO_STUB, &xf)) {
					printf ("error creating the new object in X3D_MF_TO_JS string :%s:\n",newline);
					return;
				}
                		if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(*newval), (jsint) i, xf,
                        		  JS_GET_PROPERTY_STUB, setSF_in_MF, JSPROP_ENUMERATE)) {
                        		printf( "JS_DefineElement failed for arg %u .\n", i);
                        		return;
                		}
				} else {
					/* printf ("X3DMF, ignoring NULL node here \n"); */
				}
			}
		} break;


		case FIELDTYPE_MFString: {
			struct Multi_String* MCptr;
			char newline[100];
			jsval xf;

			MCptr = (struct Multi_String *) Data;
			for (i=0; i<MCptr->n; i++) {
				#ifdef JSVRMLCLASSESVERBOSE
				printf ("X3D_MF_TO_JS, working on %d of %d, p %u\n",i, MCptr->n, MCptr->p[i]);
				#endif

				if (((struct Uni_String *)MCptr->p[i])->strptr != NULL)
					sprintf (newline,"new String('%s')", ((struct Uni_String *)MCptr->p[i])->strptr);	
				else sprintf (newline,"new String('(NULL)')");	

				#ifdef JSVRMLCLASSESVERBOSE
				printf ("X3D_MF_TO_JS, we have a new script to evaluate: \"%s\"\n",newline);
				#endif

				if (!JS_EvaluateScript(cx, JSVAL_TO_OBJECT(*newval), newline, (int) strlen(newline), FNAME_STUB, LINENO_STUB, &xf)) {
					printf ("error creating the new object in X3D_MF_TO_JS string :%s:\n",newline);
					return;
				}
                		if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(*newval), (jsint) i, xf,
                        		  JS_GET_PROPERTY_STUB, setSF_in_MF, JSPROP_ENUMERATE)) {
                        		printf( "JS_DefineElement failed for arg %u .\n", i);
                        		return;
                		}
			}
		} break;

		case FIELDTYPE_SFImage: {
			struct Multi_Int32* MCptr;
			char newline[10000];
			jsval xf;

			/* look at the PixelTexture internals, an image is just a bunch of Int32s */
			MCptr = (struct Multi_Int32 *) Data;
			sprintf (newline, "new SFImage(");

			for (i=0; i<MCptr->n; i++) {
				char sl[20];
				sprintf (sl,"0x%x ", MCptr->p[i]);	
				strcat (newline,sl); 

				if (i != ((MCptr->n)-1)) strcat (newline,",");
				if (i == 2) strcat (newline, " new MFInt32(");

			}
			strcat (newline, "))");

			if (!JS_EvaluateScript(cx, JSVAL_TO_OBJECT(*newval), newline, (int) strlen(newline), FNAME_STUB, LINENO_STUB, &xf)) {
				printf ("error creating the new object in X3D_MF_TO_JS string :%s:\n",newline);
				return;
			}
			*newval = xf; /* save this version */
		} break;
		default: {	printf("WARNING: SHOULD NOT BE HERE! %d\n",dataType); }
	}

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("returning from X3D_MF_TO_JS\n");
	#endif
}

void
reportWarningsOn() { 
	ppjsUtils p = (ppjsUtils)gglobal()->jsUtils.prv;
	p->reportWarnings = JS_TRUE; 
}


void
reportWarningsOff() { 
	ppjsUtils p = (ppjsUtils)gglobal()->jsUtils.prv;
	p->reportWarnings = JS_FALSE; 
}


void
errorReporter(JSContext *context, const char *message, JSErrorReport *report)
{
	char *errorReport = 0;
	int len = 0, charPtrSize = (int) sizeof(char *);
	ppjsUtils p = (ppjsUtils)gglobal()->jsUtils.prv;

//printf ("*** errorReporter ***\n");

    if (!report) {
        fprintf(stderr, "%s\n", message);
        return;
    }

    /* Conditionally ignore reported warnings. */
    if (JSREPORT_IS_WARNING(report->flags) && !p->reportWarnings) {
		return;
	}

	if (report->filename == NULL) {
		len = (int) (strlen(message) + 1);
	} else {
		len = (int) ((strlen(report->filename) + 1) + (strlen(message) + 1));
	}

	errorReport = (char *) JS_malloc(context, (len + STRING) * charPtrSize);
	if (!errorReport) {
		return;
	}


    if (JSREPORT_IS_WARNING(report->flags)) {
		sprintf(errorReport,
				"%swarning in %s at line %u:\n\t%s\n",
				JSREPORT_IS_STRICT(report->flags) ? "strict " : "",
				report->filename ? report->filename : "",
				report->lineno ? report->lineno : 0,
				message ? message : "No message.");
	} else {
		sprintf(errorReport,
				"error in %s at line %u:\n\t%s\n",
				report->filename ? report->filename : "",
				report->lineno ? report->lineno : 0,
				message ? message : "No message.");
	}

	fprintf(stderr, "Javascript -- %s", errorReport);

	JS_free(context, errorReport);
}


/* SFNode - find the fieldOffset pointer for this field within this node */
static int *getFOP (struct X3D_Node *node, const char *str) {
	int *fieldOffsetsPtr;


	if (node != NULL) {
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("...getFOP... it is a %s\n",stringNodeType(node->_nodeType));
		#endif

                fieldOffsetsPtr = (int *) NODE_OFFSETS[node->_nodeType];
                /*go thru all field*/
		/* what we have is a list of 4 numbers, representing:
        		FIELDNAMES__parentResource, offsetof (struct X3D_Anchor, __parenturl),  FIELDTYPE_SFString, KW_initializeOnly,
		*/

                while (*fieldOffsetsPtr != -1) {
			#ifdef JSVRMLCLASSESVERBOSE
                        printf ("getFOP, looking at field %s type %s to match %s\n",FIELDNAMES[*fieldOffsetsPtr],FIELDTYPES[*(fieldOffsetsPtr+2)],str); 
			#endif

#if TRY_PROTO_FIX
			if( 0 == strcmp("FreeWRL_PROTOInterfaceNodes",FIELDNAMES[*fieldOffsetsPtr])) {

  				#ifdef JSVRMLCLASSESVERBOSE
				printf ("%s:%d Mangle %s before trying to match %s\n",__FILE__,__LINE__,FIELDNAMES[*fieldOffsetsPtr],str);
  				#endif
				int i ;
				char *name, *str1, *token;
				char *saveptr1 = NULL;

				for (i=0; i < X3D_GROUP(node)->FreeWRL_PROTOInterfaceNodes.n; i++) {
	       				name = parser_getNameFromNode(X3D_GROUP(node)->FreeWRL_PROTOInterfaceNodes.p[i]);
					
					#ifdef JSVRMLCLASSESVERBOSE
					dump_scene(stdout,0,X3D_GROUP(node)->FreeWRL_PROTOInterfaceNodes.p[i]);
					printf ("%s:%d dummy name=%s\n",__FILE__,__LINE__,name);
					#endif

					str1 = malloc(1+strlen(name));
					strcpy(str1,name) ;
					/* discard Proto_0xnnnnn_*/
					token = strtok_r(str1, "_", &saveptr1);
					str1 = NULL;
					token = strtok_r(str1, "_", &saveptr1);
					name = saveptr1 ;

					#ifdef JSVRMLCLASSESVERBOSE
					printf ("%s:%d test indirect match %s %s\n",__FILE__,__LINE__,str,parser_getNameFromNode(X3D_GROUP(node)->FreeWRL_PROTOInterfaceNodes.p[i])) ;
					#endif
					if (strcmp(str,name) == 0) {
						#ifdef JSVRMLCLASSESVERBOSE
						printf ("getFOP, found entry for %s, it is %u (%p)\n",str
							,X3D_GROUP(node)->FreeWRL_PROTOInterfaceNodes.p[i]
							,X3D_GROUP(node)->FreeWRL_PROTOInterfaceNodes.p[i]) ;
						#endif
						return X3D_GROUP(node)->FreeWRL_PROTOInterfaceNodes.p[i] ;
					}
				}
			} else {
#endif
				/* skip any fieldNames starting with an underscore, as these are "internal" ones */
				/* There is in fact nothing in this function that actually enforces this, which is good!! */
				if (strcmp(str,FIELDNAMES[*fieldOffsetsPtr]) == 0) {
					#ifdef JSVRMLCLASSESVERBOSE
					printf ("getFOP, found entry for %s, it is %u (%p)\n",str,fieldOffsetsPtr,fieldOffsetsPtr);
					#endif
					return fieldOffsetsPtr;
				}
#if TRY_PROTO_FIX
  			}
#endif

			fieldOffsetsPtr += 5;
		}

		/* failed to find field?? */
		#if TRACK_FIFO_MSG
		printf ("getFOP, could not find field \"%s\" in nodeType \"%s\"\n", str, stringNodeType(node->_nodeType));
		#endif
	} else {
		printf ("getFOP, passed in X3D node was NULL!\n");
	}
	return NULL;
}


/* getter for SFNode accesses */
static JSBool getSFNodeField (JSContext *context, JSObject *obj, jsid id, jsval *vp) {
	JSString *_idStr;
	char *_id_c;
        SFNodeNative *ptr;
	int *fieldOffsetsPtr;
	struct X3D_Node *node;

	/* NOTE - caller is (eventually) a JS class constructor, no need to BeginRequest */

	/* _idStr = JS_ValueToString(context, id); */
/* #if JS_VERSION < 185 */
	/* _id_c = JS_GetStringBytes(_idStr); */
/* #else */
	/* _id_c = JS_EncodeString(context,_idStr); */
/* #endif */
#if JS_VERSION < 185
	_id_c = JS_GetStringBytes(JSVAL_TO_STRING(id));
#else
	_id_c = JS_EncodeString(context,JSID_TO_STRING(id));
#endif

	
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("\ngetSFNodeField called on name %s object %u\n",_id_c, obj);
	#endif

        if ((ptr = (SFNodeNative *)JS_GetPrivate(context, obj)) == NULL) {
                printf( "JS_GetPrivate failed in getSFNodeField.\n");
#if JS_VERSION >= 185
		JS_free(context,_id_c);
#endif
                return JS_FALSE;
        }
	node = X3D_NODE(ptr->handle);

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("getSFNodeField, got node %u for field %s object %u\n",node,_id_c, obj);
	printf ("getSFNodeField, got node %p for field %s object %p\n",node,_id_c, obj);
	#endif

	if (node == NULL) {
		printf ("getSFNodeField, can not set field \"%s\", NODE is NULL!\n",_id_c);
#if JS_VERSION >= 185
		JS_free(context,_id_c);
#endif
		return JS_FALSE;
	}
#if USE_OSC
	/* Is this a ring buffer item? */
	fieldOffsetsPtr = getFOP(ptr->handle,"FIFOsize");
	if (fieldOffsetsPtr == NULL) {
		#if TRACK_FIFO_MSG
		printf("getSFNodeField : This is not a ringBuffer type\n");
		#endif
	} else {
		struct X3D_OSC_Sensor *OSCnode ;
		OSCnode = (struct X3D_OSC_Sensor *) X3D_NODE(ptr->handle);
		char *_id_buffer_c = NULL ;
		RingBuffer * buffer ;

		int iVal ;
		float fVal ;
		char * strPtr ;

		#if TRACK_FIFO_MSG
		printf("getSFNodeField : This could be a ringBuffer type (found FIFOsize)\n");
		#endif

		if (0 == strcmp(_id_c,"int32Inp")) {
			#if TRACK_FIFO_MSG
			printf("getSFNodeField %d : ptr->handle=%p (which corresponds to realnode in scenegraph/Component_Networking.c,314) node=%p see X3D_NODE(ptr->handle)\n",__LINE__,ptr->handle , node);
			#endif
			/* fieldOffsetsPtr = getFOP(ptr->handle,_id_buffer_c); */
			buffer = (RingBuffer *) OSCnode->_int32InpFIFO ;
			#if TRACK_FIFO_MSG
			printf("getSFNodeField %d : buffer=%p\n",__LINE__,buffer) ;
			#endif

			if (!RingBuffer_testEmpty(buffer)) {
				_id_buffer_c = "_int32InpFIFO" ;
				iVal = RingBuffer_pullUnion(buffer)->i ;
				#if TRACK_FIFO_MSG
				printf("getSFNodeField %d : iVal=%d\n",__LINE__,iVal);
				#endif

				*vp = INT_TO_JSVAL(iVal) ;
				return JS_TRUE;
			} else {
				#if TRACK_FIFO_MSG
				printf("but the buffer is empty\n") ;
				#endif
			}
		} else if (0 == strcmp(_id_c,"floatInp")) {
			#if TRACK_FIFO_MSG
			printf("getSFNodeField %d : ptr->handle=%p (which corresponds to realnode in scenegraph/Component_Networking.c,314) node=%p see X3D_NODE(ptr->handle)\n",__LINE__,ptr->handle , node);
			#endif
			buffer = (RingBuffer *) OSCnode->_floatInpFIFO ;
			#if TRACK_FIFO_MSG
			printf("getSFNodeField %d : buffer=%p\n",__LINE__,buffer) ;
			#endif

			if (!RingBuffer_testEmpty(buffer)) {
				_id_buffer_c = "_floatInpFIFO" ;
				fVal = RingBuffer_pullUnion(buffer)->f ;
				#if TRACK_FIFO_MSG
				printf("getSFNodeField %d : fVal=%d\n",__LINE__,fVal);
				#endif

				JS_NewNumberValue(context,(double)fVal,vp);
				return JS_TRUE;
			} else {
				#if TRACK_FIFO_MSG
				printf("but the buffer is empty\n") ;
				#endif
			}
		} else if (0 == strcmp(_id_c,"stringInp")) {
			#if TRACK_FIFO_MSG
			printf("getSFNodeField %d : ptr->handle=%p (which corresponds to realnode in scenegraph/Component_Networking.c,314) node=%p see X3D_NODE(ptr->handle)\n",__LINE__,ptr->handle , node);
			#endif
			buffer = (RingBuffer *) OSCnode->_stringInpFIFO ;
			#if TRACK_FIFO_MSG
			printf("getSFNodeField %d : buffer=%p\n",__LINE__,buffer) ;
			#endif

			if (!RingBuffer_testEmpty(buffer)) {
				_id_buffer_c = "_stringInpFIFO" ;
				strPtr = (char *) RingBuffer_pullUnion(buffer)->p ;
				#if TRACK_FIFO_MSG
				printf("getSFNodeField %d : strPtr=%s\n",__LINE__,strPtr);
				#endif

				*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(context,strPtr));
				return JS_TRUE;
			} else {
				#if TRACK_FIFO_MSG
				printf("but the buffer is empty\n") ;
				#endif
			}
		} else {
			#if TRACK_FIFO_MSG
			printf("but this variable itself (%s) is not a ring buffer item\n",_id_c) ;
			#endif
		}
	}
#endif

	/* get the table entry giving the type, offset, etc. of this field in this node */
	fieldOffsetsPtr = getFOP(ptr->handle,_id_c);
	if (fieldOffsetsPtr == NULL) {
#if JS_VERSION >= 185
		JS_free(context,_id_c);
#endif
		return JS_FALSE;
	}
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("ptr=%p _id_c=%s node=%p node->_nodeType=%d stringNodeType=%s\n",ptr,_id_c,node,node->_nodeType,stringNodeType(node->_nodeType)) ;
	printf ("getSFNodeField, fieldOffsetsPtr is %d for node %u, field %s\n",fieldOffsetsPtr, ptr->handle, _id_c);
	#endif
#if JS_VERSION >= 185
	JS_free(context,_id_c);  /* _id_c is not used beyond this point in this fn */
#endif


	/* now, we have an X3D node, offset, type, etc. Just get the value from memory, and move
	   it to javascript. Kind of like: void getField_ToJavascript (int num, int fromoffset) 
	   for Routing. */

	/* fieldOffsetsPtr points to a 4-number line like:
		FIELDNAMES__parentResource, offsetof (struct X3D_Anchor, __parenturl),  FIELDTYPE_SFString, KW_initializeOnly */
        switch (*(fieldOffsetsPtr+2)) {
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFFloat:
		case FIELDTYPE_SFTime:
		case FIELDTYPE_SFDouble:
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFString:
			X3D_ECMA_TO_JS(context, offsetPointer_deref (void *, node, *(fieldOffsetsPtr+1)),
				returnElementLength(*(fieldOffsetsPtr+2)), *(fieldOffsetsPtr+2), vp);
			break;
		case FIELDTYPE_SFColor:
		case FIELDTYPE_SFNode:
		case FIELDTYPE_SFVec2f:
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFVec3d:
		case FIELDTYPE_SFRotation:
			X3D_SF_TO_JS(context, obj, offsetPointer_deref (void *, node, *(fieldOffsetsPtr+1)),
				returnElementLength(*(fieldOffsetsPtr+2)) * returnElementRowSize(*(fieldOffsetsPtr+2)) , *(fieldOffsetsPtr+2), vp);
			break;
		case FIELDTYPE_MFColor:
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFVec2f:
		case FIELDTYPE_MFFloat:
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFInt32:
		case FIELDTYPE_MFString:
		case FIELDTYPE_MFNode:
		case FIELDTYPE_MFRotation:
		case FIELDTYPE_SFImage:
			X3D_MF_TO_JS(context, obj, offsetPointer_deref (void *, node, *(fieldOffsetsPtr+1)), *(fieldOffsetsPtr+2), vp, 
				(char *)FIELDNAMES[*(fieldOffsetsPtr+0)]);
			break;
		default: printf ("unhandled type FIELDTYPE_ %d in getSFNodeField\n", *(fieldOffsetsPtr+2)) ;
		return JS_FALSE;
	}

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("end of getSFNodeField\n");
	#endif

	return JS_TRUE;
}

/* setter for SFNode accesses */
#if JS_VERSION < 185
JSBool setSFNodeField (JSContext *context, JSObject *obj, jsid id, jsval *vp) {
#else
JSBool setSFNodeField (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	char *_id_c;
        SFNodeNative *ptr;
	int *fieldOffsetsPtr;
	struct X3D_Node *node;

	/* get the id field... */

#if JS_VERSION < 185
	_id_c = JS_GetStringBytes(JSVAL_TO_STRING(id));
#else
	_id_c = JS_EncodeString(context,JSID_TO_STRING(id));
#endif
	
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("\nsetSFNodeField called on name %s object %u, jsval %u\n",_id_c, obj, *vp);
	#endif

	/* get the private data. This will contain a pointer into the FreeWRL scenegraph */
        if ((ptr = (SFNodeNative *)JS_GetPrivate(context, obj)) == NULL) {
                printf( "JS_GetPrivate failed in setSFNodeField.\n");
#if JS_VERSION >= 185
		JS_free(context,_id_c);
#endif
                return JS_FALSE;
        }

	/* get the X3D Scenegraph node pointer from this Javascript SFNode node */
	node = (struct X3D_Node *) ptr->handle;

	if (node == NULL) {
		printf ("setSFNodeField, can not set field \"%s\", NODE is NULL!\n",_id_c);
#if JS_VERSION >= 185
		JS_free(context,_id_c);
#endif
		return JS_FALSE;
	}

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("so then, we have a node of type %s\n",stringNodeType(node->_nodeType));
	#endif

	#if USE_OSC
	#ifdef JSVRMLCLASSESVERBOSE
	/* Is this a ring buffer item? */
	fieldOffsetsPtr = getFOP(ptr->handle,"FIFOsize");
	#if TRACK_FIFO_MSG
	if (fieldOffsetsPtr == NULL) {
		printf("setSFNodeField : This is not a ringBuffer type\n");
	} else {
		printf("setSFNodeField : This is a ringBuffer type\n");
	}
	#endif
	#endif
	#endif

	/* get the table entry giving the type, offset, etc. of this field in this node */
	fieldOffsetsPtr = getFOP(ptr->handle,_id_c);
#if JS_VERSION >= 185
	JS_free(context,_id_c); /* _id_c is not used beyond this point in this fn */
#endif
	if (fieldOffsetsPtr == NULL) {
		return JS_FALSE;
	}

	/* now, we have an X3D node, offset, type, etc. Just get the value from Javascript, and move
	   it to the X3D Scenegraph. */

	/* fieldOffsetsPtr points to a 4-number line like:
		FIELDNAMES__parentResource, offsetof (struct X3D_Anchor, __parenturl),  FIELDTYPE_SFString, KW_initializeOnly */
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("and a field type of %s\n",FIELDTYPES[*(fieldOffsetsPtr+2)]);
	#endif

        switch (*(fieldOffsetsPtr+2)) {
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFFloat:
		case FIELDTYPE_SFTime:
		case FIELDTYPE_SFDouble:
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFString:
			JS_ECMA_TO_X3D(context, ((void *)( ((unsigned char *) node) + *(fieldOffsetsPtr+1))),
				returnElementLength(*(fieldOffsetsPtr+2)), *(fieldOffsetsPtr+2), vp);
			break;
		case FIELDTYPE_SFColor:
		case FIELDTYPE_SFNode:
		case FIELDTYPE_SFVec2f:
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFVec3d:
		case FIELDTYPE_SFRotation:
			JS_SF_TO_X3D(context, ((void *)( ((unsigned char *) node) + *(fieldOffsetsPtr+1))),
				returnElementLength(*(fieldOffsetsPtr+2)) * returnElementRowSize(*(fieldOffsetsPtr+2)) , *(fieldOffsetsPtr+2), vp);
			break;
		case FIELDTYPE_MFColor:
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFVec2f:
		case FIELDTYPE_MFFloat:
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFInt32:
		case FIELDTYPE_MFString:
		case FIELDTYPE_MFNode:
		case FIELDTYPE_MFRotation:
		case FIELDTYPE_SFImage:
			JS_MF_TO_X3D(context, obj, ((void *)( ((unsigned char *) node) + *(fieldOffsetsPtr+1))), *(fieldOffsetsPtr+2), vp);
			break;
		default: printf ("unhandled type in setSFNodeField\n");
		return JS_FALSE;
	}

	/* tell the X3D Scenegraph that something has changed in Kansas */
	update_node(node);

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("end of setSFNodeField\n");
	#endif


	return JS_TRUE;
}

/* for SFNodes, go through and insure that all properties are defined for the specific node type */
int JS_DefineSFNodeSpecificProperties (JSContext *context, JSObject *object, struct X3D_Node * ptr) {
	int *fieldOffsetsPtr;
	jsval rval = INT_TO_JSVAL(0);
	uintN attrs = JSPROP_PERMANENT 
		| JSPROP_ENUMERATE 
#ifdef JSPROP_EXPORTED
		| JSPROP_EXPORTED 
#endif
	/*	| JSPROP_INDEX */
		;
	char *name;
	SFNodeNative *nodeNative;
	#ifdef JSVRMLCLASSESVERBOSE
	char *nodeName;
	struct X3D_Node *confirmNode;
	#endif

	/* NOTE - caller of this function is a class constructor, no need to worry about Requests */


	#ifdef JSVRMLCLASSESVERBOSE
        nodeName = parser_getNameFromNode(ptr) ; /* vi +/dump_scene src/lib/scenegraph/GeneratedCode.c */
        if (nodeName == NULL) {
		printf ("\nStart of JS_DefineSFNodeSpecificProperties for '---' ... working on node %u object %u (%p,%p)\n",ptr,object,ptr,object);
        } else {
		printf ("\nStart of JS_DefineSFNodeSpecificProperties for '%s' ... working on node %u object %u (%p,%p)\n",nodeName,ptr,object,ptr,object);
		confirmNode = parser_getNodeFromName(nodeName);
        	if (confirmNode == NULL) {
			printf("RoundTrip failed : ptr (%p) -> nodeName (%s) -----\n",ptr,nodeName) ;
		} else {
			printf("RoundTrip OK : ptr (%p) -> nodeName (%s) -> confirmNode (%p)\n",ptr,nodeName,confirmNode) ;
		}
        }
	#endif 

	if (ptr != NULL) {
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("...JS_DefineSFNodeSpecificProperties... it is a %s\n",stringNodeType(ptr->_nodeType));
		#endif

		/* have we already done this for this node? We really do not want to do this again */
		if ((nodeNative = (SFNodeNative *)JS_GetPrivate(context,object)) == NULL) {
			printf ("JS_DefineSFNodeSpecificProperties, can not get private for a SFNode!\n");
			return JS_FALSE;
		}
		if (nodeNative->fieldsExpanded) {
			#ifdef JSVRMLCLASSESVERBOSE
			printf ("JS_DefineSFNodeSpecificProperties, already done for node\n");
			#endif

			return JS_TRUE;
		}

                fieldOffsetsPtr = (int *) NODE_OFFSETS[ptr->_nodeType];
                /*go thru all field*/
		/* what we have is a list of 4 numbers, representing:
        		FIELDNAMES__parentResource, offsetof (struct X3D_Anchor, __parenturl),  FIELDTYPE_SFString, KW_initializeOnly,
		*/

                while (*fieldOffsetsPtr != -1) {
                        /* fieldPtr=(char*)structptr+(*(fieldOffsetsPtr+1)); */
			#ifdef JSVRMLCLASSESVERBOSE
                        printf ("looking at field %s type %s\n",FIELDNAMES[*fieldOffsetsPtr],FIELDTYPES[*(fieldOffsetsPtr+2)]); 
			#endif
			
#if USE_OSC
			if( 0 == strcmp("FreeWRL_PROTOInterfaceNodes",FIELDNAMES[*fieldOffsetsPtr])) {

				#ifdef JSVRMLCLASSESVERBOSE
				printf ("%s:%d Mangle %s before calling JS_DefineProperty ....\n",__FILE__,__LINE__,FIELDNAMES[*fieldOffsetsPtr]);
				#endif
				int i ;
				char *str1, *token;
				char *saveptr1 = NULL;

				for (i=0; i < X3D_GROUP(ptr)->FreeWRL_PROTOInterfaceNodes.n; i++) {
					rval = INT_TO_JSVAL(*fieldOffsetsPtr);
        				name = parser_getNameFromNode(X3D_GROUP(ptr)->FreeWRL_PROTOInterfaceNodes.p[i]);
					
					#ifdef JSVRMLCLASSESVERBOSE
					dump_scene(stdout,0,X3D_GROUP(ptr)->FreeWRL_PROTOInterfaceNodes.p[i]);
					printf ("%s:%d dummy name=%s\n",__FILE__,__LINE__,name);
					#endif

					str1 = malloc(1+strlen(name));
					strcpy(str1,name) ;
					/* discard Proto_0xnnnnn_*/
					token = strtok_r(str1, "_", &saveptr1);
					str1 = NULL;
					token = strtok_r(str1, "_", &saveptr1);
					name = saveptr1 ;

					#ifdef JSVRMLCLASSESVERBOSE
					printf ("%s:%d would call JS_DefineProperty on (context=%p, object=%p, name=%s, rval=%p), setting getSFNodeField, setSFNodeField\n",__FILE__,__LINE__,context,object,name,rval);
					#endif
					if (!JS_DefineProperty(context, object,  name, rval, getSFNodeField, setSFNodeField, attrs)) {
						printf("JS_DefineProperty failed for \"%s\" in JS_DefineSFNodeSpecificProperties.\n", name);
						return JS_FALSE;
					}
				}
			} else if (FIELDNAMES[*fieldOffsetsPtr][0] != '_') {
#else
			if (FIELDNAMES[*fieldOffsetsPtr][0] != '_') {
#endif
				/* skip any fieldNames starting with an underscore, as these are "internal" ones */
				name = (char *)FIELDNAMES[*fieldOffsetsPtr];
				rval = INT_TO_JSVAL(*fieldOffsetsPtr);

				/* is this an initializeOnly property? */
				/* lets not do this, ok? 
				if ((*(fieldOffsetsPtr+3)) == KW_initializeOnly) attrs |= JSPROP_READONLY;
				*/

				#ifdef JSVRMLCLASSESVERBOSE
				printf ("calling JS_DefineProperty on (context=%p, object=%p, name=%s, rval=%p), setting getSFNodeField, setSFNodeField\n",context,object,name,rval);
				#endif

        			if (!JS_DefineProperty(context, object,  name, rval, getSFNodeField, setSFNodeField, attrs)) {
        			        printf("JS_DefineProperty failed for \"%s\" in JS_DefineSFNodeSpecificProperties.\n", name);
        			        return JS_FALSE;
        			}
			}
			fieldOffsetsPtr += 5;
		}

		/* set a flag indicating that we have been here already */
		nodeNative->fieldsExpanded = TRUE;
	}
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("JS_DefineSFNodeSpecificProperties, returning TRUE\n");
	#endif

	return TRUE;
}


/********************************************************************************************/
/* new addition April 2009. It was noted that the following code would not send an event to
   FreeWRL:
#VRML V2.0 utf8
      DEF DisplayScript Script {
        eventOut MFString display_string

        url [ "javascript:
          function eventsProcessed () {
		display_string[7] = ' ';
          }
        "]
      }


Shape {geometry DEF Display Text {}}
    ROUTE DisplayScript.display_string TO Display.set_string

(it would if the assignment was display_string = new MFString(...) )

But, this property check gets called on the equals. Lets figure out how to indicate that the
holding object needs to route to FreeWRL... */


#define SET_TOUCHED_TYPE_MF_A(thisMFtype,thisSFtype) \
	else if (JS_InstanceOf (cx, obj, &thisMFtype##Class, NULL)) {\
		jsval mainElement;\
		thisSFtype##Native *ptr; \
\
		if (!JS_GetElement(cx, obj, num, &mainElement)) { \
			printf ("JS_GetElement failed for %d in get_valueChanged_flag\n",num); \
			return JS_FALSE; \
		} \
\
		if ((ptr = (thisSFtype##Native *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(mainElement))) == NULL) {\
			printf( "JS_GetPrivate failed in assignCheck.\n"); \
			return JS_FALSE; \
		} else { \
			/* printf ("got private for MFVec3f, doing it...\n"); */ \
			ptr->valueChanged++; \
		} \
		return JS_TRUE; \
        }


#if JS_VERSION < 185
JSBool js_SetPropertyCheck (JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyCheck(JSContext *cx, JSObject *obj, jsid iid, JSBool strict, jsval *vp) {
#endif
	int num=0;
#if JS_VERSION >= 185
	jsval id;
	if (!JS_IdToValue(cx,iid,&id)) {
		printf("js_SetPropertyCheck: JS_IdToValue failed.\n");
		return JS_FALSE;
	}
#endif

#ifdef JAVASCRIPTVERBOSE
	char *_id_c = "(no value in string)";
	/* get the id field... */
	if (JSVAL_IS_STRING(id)) { 
/*
#if JS_VERSION < 185
		_id_c = JS_GetStringBytes(JSVAL_TO_STRING(id)); 
#else
		_id_c = JS_EncodeString(cx,JSVAL_TO_STRING(id)); 
#endif
        	 printf ("hmmm...js_SetPropertyCheck called on string \"%s\" object %u, jsval %u\n",_id_c, obj, *vp); */
	} else if (JSVAL_IS_DOUBLE(id)) {
#if JS_VERSION < 185
		_id_c = JS_GetStringBytes(JSVAL_TO_STRING(id)); 
#else
		_id_c = JS_EncodeString(cx,JSVAL_TO_STRING(id)); 
#endif
        	printf ("\n...js_SetPropertyCheck called on double %s object %u, jsval %u\n",_id_c, obj, *vp);
#if JS_VERSION >= 185
		JS_free(cx,_id_c);
#endif
	} else if (JSVAL_IS_INT(id)) {
		num = JSVAL_TO_INT(id);
        	printf ("\n...js_SetPropertyCheck called on number %d object %u, jsval %u\n",num, obj, *vp); 
	} else {
        	printf ("hmmm...js_SetPropertyCheck called on unknown type of object %u, jsval %u\n", obj, *vp);
	}
#endif

	/* lets worry about the MFs containing ECMAs here - MFFloat MFInt32 MFTime MFString MFBool */

	if (JS_InstanceOf (cx, obj, &MFStringClass, NULL)) {
		SET_MF_ECMA_HAS_CHANGED;
		return JS_TRUE;
	}
	else if (JS_InstanceOf (cx, obj, &MFFloatClass, NULL)) {
		SET_MF_ECMA_HAS_CHANGED;
		return JS_TRUE;
	}
	else if (JS_InstanceOf (cx, obj, &MFInt32Class, NULL)) {
		SET_MF_ECMA_HAS_CHANGED;
		return JS_TRUE;
	}

#ifdef NEWCLASSES
	else if (JS_InstanceOf (cx, obj, &MFBoolClass, NULL)) {
		SET_MF_ECMA_HAS_CHANGED;
		return JS_TRUE;
	}
#endif

        SET_TOUCHED_TYPE_MF_A(MFRotation,SFRotation)
        SET_TOUCHED_TYPE_MF_A(MFNode,SFNode)
        SET_TOUCHED_TYPE_MF_A(MFVec2f,SFVec2f)
        SET_TOUCHED_TYPE_MF_A(MFVec3f,SFVec3f)
        /* SET_TOUCHED_TYPE_MF_A(MFImage,SFImage)  */
        SET_TOUCHED_TYPE_MF_A(MFColor,SFColor)
        /* SET_TOUCHED_TYPE_MF_A(MFColorRGBA,SFColorRGBA) */


        #ifdef JSVRMLCLASSESVERBOSE
	printf ("this is a class of "); printJSNodeType (cx,obj);
        #endif
	
	return JS_TRUE;

}

/****************************************************************************/

#if JS_VERSION < 185
JSBool js_GetPropertyDebug (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_GetPropertyDebug (JSContext *context, JSObject *obj, jsid iid, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	char *_id_c = "(no value in string)";
	int num;
	/* get the id field... */
#if JS_VERSION >= 185
	jsval id;
	if (!JS_IdToValue(context,iid,&id)) {
		printf("js_GetPropertyDebug: JS_IdToValue failed -- NOT returning false\n");
	}
#endif

	if (JSVAL_IS_STRING(id)) { 
#if JS_VERSION < 185
		_id_c = JS_GetStringBytes(JSVAL_TO_STRING(id)); 
#else
		_id_c = JS_EncodeString(context,JSVAL_TO_STRING(id)); 
#endif
        	printf ("\n...js_GetPropertyDebug called on string \"%s\" object %u, jsval %lu\n",_id_c, (unsigned int) obj, *vp);
#if JS_VERSION >= 185
		JS_free(context,_id_c);
#endif
	} else if (JSVAL_IS_INT(id)) {
		num = JSVAL_TO_INT(id);
        	printf ("\n...js_GetPropertyDebug called on number %d object %u, jsval %lu\n",num, (unsigned int) obj, *vp);
	} else {
        	printf ("\n...js_GetPropertyDebug called on unknown type of object %u, jsval %lu\n", (unsigned int) obj, *vp);
	}
	#endif
	return JS_TRUE;
}

#ifdef JSVRMLCLASSESVERBOSE 
#if JS_VERSION < 185
void js_SetPropertyDebugWrapped (JSContext *context, JSObject *obj, jsval id, jsval *vp,char *debugString) {
#else
void js_SetPropertyDebugWrapped (JSContext *context, JSObject *obj, jsid iid, jsval *vp,char *debugString) {
#endif
	char *_id_c = "(no value in string)";
	int num;
#if JS_VERSION >= 185
	jsval id;
	if (!JS_IdToValue(context,iid,&id)) {
		printf("js_GetPropertyDebug: JS_IdToValue failed\n");
	}
#endif

	/* get the id field... */
	if (JSVAL_IS_STRING(id)) { 
#if JS_VERSION < 185
		_id_c = JS_GetStringBytes(JSVAL_TO_STRING(id)); 
#else
		_id_c = JS_EncodeString(context,JSVAL_TO_STRING(id)); 
#endif
        	printf ("\n...js_SetPropertyDebug%s called on string \"%s\" object %p, jsval %lu\n",debugString,_id_c, obj, *vp);
#if JS_VERSION >= 185
		JS_free(context,_id_c);
#endif
	} else if (JSVAL_IS_INT(id)) {
		num = JSVAL_TO_INT(id);
        	printf ("\n...js_SetPropertyDebug%s called on number %d object %p, jsval %lu\n",debugString,num, obj, *vp);
	} else {
        	printf ("\n...js_SetPropertyDebug%s called on unknown type of object %p, jsval %lu\n",debugString, obj, *vp);
	}
}
#endif

#if JS_VERSION < 185
JSBool js_SetPropertyDebug (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"");
	#endif
	return JS_TRUE;
}

#if JS_VERSION < 185
JSBool js_SetPropertyDebug1 (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug1 (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"1");
	#endif
	return JS_TRUE;
}
#if JS_VERSION < 185
JSBool js_SetPropertyDebug2 (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug2 (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"2");
	#endif
	return JS_TRUE;
}
#if JS_VERSION < 185
JSBool js_SetPropertyDebug3 (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug3 (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"3");
	#endif
	return JS_TRUE;
}
#if JS_VERSION < 185
JSBool js_SetPropertyDebug4 (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug4 (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"4");
	#endif
	return JS_TRUE;
}
#if JS_VERSION < 185
JSBool js_SetPropertyDebug5 (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug5 (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"5");
	#endif
	return JS_TRUE;
}
#if JS_VERSION < 185
JSBool js_SetPropertyDebug6 (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug6 (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"6");
	#endif
	return JS_TRUE;
}
#if JS_VERSION < 185
JSBool js_SetPropertyDebug7 (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug7 (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"7");
	#endif
	return JS_TRUE;
}
#if JS_VERSION < 185
JSBool js_SetPropertyDebug8 (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug8 (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"8");
	#endif
	return JS_TRUE;
}
#if JS_VERSION < 185
JSBool js_SetPropertyDebug9 (JSContext *context, JSObject *obj, jsval id, jsval *vp) {
#else
JSBool js_SetPropertyDebug9 (JSContext *context, JSObject *obj, jsid id, JSBool strict, jsval *vp) {
#endif
	#ifdef JSVRMLCLASSESVERBOSE 
	js_SetPropertyDebugWrapped(context,obj,id,vp,"9");
	#endif
	return JS_TRUE;
}

#endif /* HAVE_JAVASCRIPT */
