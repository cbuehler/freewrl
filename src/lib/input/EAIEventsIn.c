/*
=INSERT_TEMPLATE_HERE=

$Id: EAIEventsIn.c,v 1.83 2012/05/17 02:38:56 crc_canada Exp $

Handle incoming EAI (and java class) events with panache.

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
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CProto.h"
#include "../vrml_parser/CParse.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../world_script/fieldGet.h"

#include "../input/EAIHeaders.h"
#include "../world_script/fieldSet.h"
#include "../scenegraph/Viewer.h"
#include "../opengl/OpenGL_Utils.h"
#include "../scenegraph/RenderFuncs.h"
#include "../opengl/Textures.h"
#include "../opengl/OpenGL_Utils.h"
#include "../x3d_parser/X3DParser.h"
#include "../vrml_parser/CRoutes.h"

#include "EAIHelpers.h"

#include <ctype.h> /* FIXME: config armor */

#define EAI_BUFFER_CUR tg->EAIServ.EAIbuffer[bufPtr]

/* used for loadURL */
//struct X3D_Anchor EAI_AnchorNode;
//int waiting_for_anchor = FALSE;

static void makeFIELDDEFret(int, int);
static void handleRoute (char command, char *bufptr, int repno);
static void handleGETNODE (char *bufptr, int repno);
static void handleGETROUTES (char *bufptr, int repno);
static void handleGETEAINODETYPE (char *bufptr, int repno);
extern void dump_scene (FILE *fp, int level, struct X3D_Node* node); // in GeneratedCode.c


/******************************************************************************
*
* EAI_parse_commands
*
* there can be many commands waiting, so we loop through commands, and return
* a status of EACH command
*
* a Command starts off with a sequential number, a space, then a letter indicating
* the command, then the parameters of the command.
*
* the command names are #defined at the start of this file.
*
* some commands have sub commands (eg, get a value) to indicate data types,
* (eg, EAI_SFFLOAT); these sub types are indicated with a lower case letter; again,
* look to the top of this file for the #defines
*
*********************************************************************************/


typedef struct pEAIEventsIn{
int oldCount; //=0;
int waiting_for_anchor; // = FALSE;
struct X3D_Anchor EAI_AnchorNode;
}* ppEAIEventsIn;
void *EAIEventsIn_constructor()
{
	void *v = malloc(sizeof(struct pEAIEventsIn));
	memset(v,0,sizeof(struct pEAIEventsIn));
	return v;
}
void EAIEventsIn_init(struct tEAIEventsIn* t)
{
	//public
	//struct X3D_Anchor EAI_AnchorNode; //null ok?

	//private
	t->prv = EAIEventsIn_constructor();
	{
		ppEAIEventsIn p = (ppEAIEventsIn)t->prv;
		p->waiting_for_anchor = FALSE;
		p->oldCount=0;
	}
}

struct X3D_Anchor* get_EAIEventsIn_AnchorNode()
{
	ppEAIEventsIn p = (ppEAIEventsIn)gglobal()->EAIEventsIn.prv;
	return (struct X3D_Anchor*)&p->EAI_AnchorNode;
}

#if !defined(EXCLUDE_EAI)
void EAI_parse_commands () {
	/* char buf[EAIREADSIZE];*/
	char ctmp[EAIREADSIZE];	/* temporary character buffer*/
	char dtmp[EAIREADSIZE];	/* temporary character buffer*/
	struct X3D_Group *retGroup;

	int count;
	char command;
	int bufPtr;		/* where we are in the EAI input buffer */
	
	int ra,rb,rd;	/* temps*/
	int rc;
	int tmp_a, tmp_b, tmp_c;

	int scripttype;
	char *EOT;		/* ptr to End of Text marker*/
	int retint;		/* used for getting retval for sscanf */

	struct X3D_Node *boxptr;
        int ctype;
	int xxx;

	char *dumpname ;
	int dumpfsize ;
	int dumpInt ;
	FILE *dumpfd ;
	int eaiverbose;
	ppEAIEventsIn p;
	//ppEAIServ ps;
	struct tEAIHelpers *th;
	ttglobal tg = gglobal();
	p = (ppEAIEventsIn)tg->EAIEventsIn.prv;
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;
	th = &tg->EAIHelpers;
	//ps = tg->EAIServ.prv;

	/* initialization */
	bufPtr = 0;

	/* output buffer - start off with it this size */
	th->outBufferLen = EAIREADSIZE;
	th->outBuffer = MALLOC(char *, th->outBufferLen);

	while (EAI_BUFFER_CUR> 0) {
		if (eaiverbose) {
			printf ("EAI_parse_commands:start of while loop, strlen %d str :%s:\n",(int)strlen((&EAI_BUFFER_CUR)),(&EAI_BUFFER_CUR));
		}

		/* step 1, get the command sequence number */
		if (sscanf ((&EAI_BUFFER_CUR),"%d",&count) != 1) {
			printf ("EAI_parse_commands, expected a sequence number on command :%s:\n",(&EAI_BUFFER_CUR));
			count = 0;
		}
		if (eaiverbose) {
			printf ("EAI - seq number %d\n",count);
		}

		if (count != (p->oldCount+1)) {
			printf ("COUNT MISMATCH, expected %d got %d\n",p->oldCount+1,count);
		}
		p->oldCount = count;


		/* step 2, skip past the sequence number */
		while (isdigit(EAI_BUFFER_CUR)) bufPtr++;
		/* if (eaiverbose) {
			printf("past sequence number, string:%s\n",(&EAI_BUFFER_CUR));
		} */

		while (EAI_BUFFER_CUR == ' ') bufPtr++;
		/* if (eaiverbose) {
			printf ("past the space, string:%s\n",(&EAI_BUFFER_CUR)); 
		} */

		/* step 3, get the command */

		command = EAI_BUFFER_CUR;
		if (eaiverbose) 
			printf ("EAI command %s (%c) strlen %d\n",eaiPrintCommand(command), command,(int)strlen(&EAI_BUFFER_CUR));

		bufPtr++;

		/* return is something like: $hand->print("RE\n$reqid\n1\n$id\n");*/
		if (eaiverbose) {
			printf ("\n... %d ",count);
		}

		switch (command) {
			case DUMPSCENE: {
				int throwAway = 0 ;
				int sendNameNotFile = 1 ;
				dumpname = TEMPNAM("/tmp","fwtmp");
				dumpfd = fopen(dumpname,"w+");
				dump_scene(dumpfd, 0, (struct X3D_Node*) rootNode());
				fflush(dumpfd) ;
				if (sendNameNotFile) {
					fclose(dumpfd) ;
					throwAway = sprintf (th->outBuffer,"RE\n%f\n%d\n%s",TickTime(),count,dumpname);
				} else {
					dumpfsize = (int) ftell(dumpfd) ;
					fseek(dumpfd, 0L, SEEK_SET) ;

					th->outBuffer = REALLOC(th->outBuffer,dumpfsize+200);
					dumpInt = sprintf (th->outBuffer,"RE\n%f\n%d\n",TickTime(),count);
					throwAway = (int) fread(th->outBuffer+dumpInt, dumpfsize, 1, dumpfd);
					dumpInt += dumpfsize;

					th->outBuffer[dumpInt] = '\0';
					fclose(dumpfd) ;
					unlink(dumpname) ;
				}
				break;
				}
			case GETRENDPROP: {
				ttglobal tg = gglobal();
				sprintf (th->outBuffer,"RE\n%f\n%d\n%s %dx%d %d %s %d %f",TickTime(),count,
					"SMOOTH",				/* Shading */
					tg->display.rdr_caps.max_texture_size, gglobal()->display.rdr_caps.max_texture_size, 	/* Texture size */	
					tg->display.rdr_caps.texture_units,				/* texture units */
					"FALSE",				/* antialiased? */
					tg->OpenGL_Utils.displayDepth,				/* bit depth of display */
					256.0					/* amount of memory left on card -
										   can not find this in OpenGL, so
										   just make it large... */
					);
				break;
				}

			case GETNAME: {
				sprintf (th->outBuffer,"RE\n%f\n%d\n%s",TickTime(),count,BrowserName);
				break;
				}
			case GETVERSION: {
				sprintf (th->outBuffer,"RE\n%f\n%d\n%s",TickTime(),count,libFreeWRL_get_version());
				break;
				}
			case GETENCODING: {
				sprintf (th->outBuffer,"RE\n%f\n%d\n%d",TickTime(),count,tg->Mainloop.currentFileVersion);
				break;
				}
			case GETCURSPEED: {
				/* get the BrowserSpeed variable updated */
				getCurrentSpeed();
				sprintf (th->outBuffer,"RE\n%f\n%d\n%f",TickTime(),count,gglobal()->Mainloop.BrowserSpeed);
				break;
				}
			case GETFRAMERATE: {
				sprintf (th->outBuffer,"RE\n%f\n%d\n%f",TickTime(),count,gglobal()->Mainloop.BrowserFPS);
				break;
				}
			case GETURL: {
				sprintf (th->outBuffer,"RE\n%f\n%d\n%s",TickTime(),count,BrowserFullPath);
				break;
				}
			case GETNODE:  {
				handleGETNODE(&EAI_BUFFER_CUR,count);
				break;
			}
			case GETROUTES:  {
				handleGETROUTES(&EAI_BUFFER_CUR,count);
				break;
			}

			case GETEAINODETYPE: {
				handleGETEAINODETYPE(&EAI_BUFFER_CUR,count);
				break;
			}

			case GETNODETYPE: {
				int cNode;
				retint = sscanf(&EAI_BUFFER_CUR,"%d",(&cNode));
				if (cNode != 0) {
        				boxptr = getEAINodeFromTable(cNode,-1);
					sprintf (th->outBuffer,"RE\n%f\n%d\n%d",TickTime(),count,getSAI_X3DNodeType (
						boxptr->_nodeType));
				} else {
					sprintf (th->outBuffer,"RE\n%f\n%d\n-1",TickTime(),count);
				}
				/* printf ("GETNODETYPE, for node %s, returns %s\n",(&EAI_BUFFER_CUR),buf); */
					
				break;
				}

			case GETFIELDTYPE:  {
				int xtmp;

				/*format int seq# COMMAND  int node#   string fieldname   string direction*/

				retint=sscanf (&EAI_BUFFER_CUR,"%d %s %s",&xtmp, ctmp,dtmp);
				if (eaiverbose) {	
					printf ("GETFIELDTYPE cptr %d %s %s\n",xtmp, ctmp, dtmp);
				}	

				EAI_GetType (xtmp, ctmp, dtmp, &ra, &rb, &rc, &rd, &scripttype, &xxx);
				sprintf (th->outBuffer,"RE\n%lf\n%d\n%d %d %d %c %d %s",TickTime(),count,(int)ra,(int)rb,(int)rc,(int)rd,
						scripttype,stringKeywordType(xxx));
				break;
				}
			case SENDEVENT:   {
				/*format int seq# COMMAND NODETYPE pointer offset data*/
				setField_FromEAI (&EAI_BUFFER_CUR);
				if (eaiverbose) {	
					printf ("after SENDEVENT, strlen %d\n",(int)strlen(&EAI_BUFFER_CUR));
				}
				break;
				}
			case CREATEVU:
			case CREATEVS: {
				/*format int seq# COMMAND vrml text     string EOT*/

				retGroup = createNewX3DNode(NODE_Group);
				if (command == CREATEVS) {
					if (eaiverbose) {	
						printf ("CREATEVS %s\n",&EAI_BUFFER_CUR);
					}	

					EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");
					/* if we do not have a string yet, we have to do this...*/
					while (EOT == NULL) {
						tg->EAIServ.EAIbuffer = read_EAI_socket(tg->EAIServ.EAIbuffer,&tg->EAIServ.EAIbufcount, &tg->EAIServ.EAIbufsize, &tg->EAIServ.EAIlistenfd);
						EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");
					}

					*EOT = 0; /* take off the EOT marker*/

					ra = EAI_CreateVrml("String",(&EAI_BUFFER_CUR),retGroup);
					/* finish this, note the pointer maths */
					bufPtr = (int) (EOT+3-tg->EAIServ.EAIbuffer);
				} else {
/*  					char *filename = MALLOC(char *,1000); */
					char *mypath;

					/* sanitize this string - remove leading and trailing garbage */
					rb = 0;
					while ((EAI_BUFFER_CUR!=0) && (EAI_BUFFER_CUR <= ' ')) bufPtr++;
					while (EAI_BUFFER_CUR > ' ') { ctmp[rb] = EAI_BUFFER_CUR; rb ++; bufPtr++; }

					/* ok, lets make a real name from this; maybe it is local to us? */
					ctmp[rb] = 0;

					/* get the current parent */
					mypath = STRDUP(ctmp);
					DEBUG_MSG("CREATEVU, mypath %s\n", mypath);

					/* and strip off the file name, leaving any path */
/* 					removeFilenameFromPath (mypath); */
					/* printf ("CREATEVU, mypath sans file: %s\n",mypath); */

					/* add the two together */
/* 					makeAbsoluteFileName(filename,mypath,ctmp); */
					/* printf ("CREATEVU, filename, %s\n",filename); */

/* 					if (eaiverbose) {	 */
/* 						printf ("CREATEVU %s\n",filename); */
/* 					}	 */

					ra = EAI_CreateVrml("URL", mypath, retGroup);
/* 					FREE_IF_NZ(filename); */
					FREE_IF_NZ(mypath);
				}

				/* printf ("ok, we are going to return the following number of nodes: %d\n",retGroup->children.n); */
				sprintf (th->outBuffer,"RE\n%f\n%d\n",TickTime(),count);
				for (rb = 0; rb < retGroup->children.n; rb++) {
					sprintf (ctmp,"%d ", registerEAINodeForAccess(X3D_NODE(retGroup->children.p[rb])));
					outBufferCat(ctmp);
				}

				markForDispose(X3D_NODE(retGroup),FALSE);
				break;
				}

			case SENDCHILD :  {
				struct X3D_Node *node;
				char *address;

				/*format int seq# COMMAND  int node#   ParentNode field ChildNode*/

				retint=sscanf (&EAI_BUFFER_CUR,"%d %d %s %d",&ra,&rb,ctmp,&rc);

				node = getEAINodeFromTable(ra,rb);
				address = getEAIMemoryPointer (ra,rb);

				if (eaiverbose) {	
					printf ("SENDCHILD Parent: %u ParentField: %u %s Child: %s\n",(unsigned int) ra, (unsigned int)rb, ctmp, dtmp);
				}	

				/* we actually let the end of eventloop code determine whether this is an add or
				   remove, or whatever - it will handle the addChildren and removeChildren well
				   so the add/remove/replace parameter of getMFNodetype is always 1 here */

				getMFNodetype (getEAINodeFromTable(rc,-1),(struct Multi_Node *)address, node, 1);

				/* tell the routing table that this node is updated - used for RegisterListeners */
				MARK_EVENT(node,getEAIActualOffset(ra,rb));

				sprintf (th->outBuffer,"RE\n%f\n%d\n0",TickTime(),count);
				break;
				}
			case REGLISTENER: {
				struct X3D_Node * node;
				int  offset;
				int directionFlag = 0;

				/*143024848 88 8 e 6*/
				retint=sscanf (&EAI_BUFFER_CUR,"%d %d %c %d",&tmp_a,&tmp_b,ctmp,&tmp_c);
				node = getEAINodeFromTable(tmp_a, tmp_b);
				offset = getEAIActualOffset(tmp_a, tmp_b);

				/* is this a script node? if so, get the actual string name in the table for this one */
				if (node->_nodeType == NODE_Script) {
					struct Shader_Script * sp;

					/* we send along the script number, not the node pointer */
					sp = (struct Shader_Script *) (X3D_SCRIPT(node)->__scriptObj);

					/* some print statements here because this is a script node 
					printf ("ah! so noderef  %d, node %u is script num %d\n",tmp_a,node,sp->num);
					printf ("this is a script node in a REGLISTENER command! %d\n",tmp_b);
					*/

					node = offsetPointer_deref(struct X3D_Node *,0,sp->num);
					directionFlag = FROM_SCRIPT;
				}

				/* so, count = query id, tmp_a pointer, tmp_b, offset, ctmp[0] type, tmp_c, length*/
				ctmp[1]=0;

				if (eaiverbose) 
					printf ("REGISTERLISTENER from %p foffset %d fieldlen %d type %s \n",
						node, offset ,tmp_c,ctmp);


				/* put the address of the listener area in a string format for registering
				   the route - the route propagation will copy data to here */

				/* set up the route from this variable to the handle Listener routine */
				if (eaiverbose)  printf ("going to register route for RegisterListener, have type %d\n",tmp_c); 

				CRoutes_Register  (1,node, offset, X3D_NODE(tg->EAIServ.EAIListenerData), 0, (int) tmp_c,(void *) 
					&EAIListener, directionFlag, (count<<8)+mapEAItypeToFieldType(ctmp[0])); /* encode id and type here*/

				sprintf (th->outBuffer,"RE\n%f\n%d\n0",TickTime(),count);
				break;
				}

			case UNREGLISTENER: {
				struct X3D_Node * node;
				int offset;
				int directionFlag = 0;

				/*143024848 88 8 e 6*/
				retint=sscanf (&EAI_BUFFER_CUR,"%d %d %c %d",&tmp_a,&tmp_b,ctmp,&tmp_c);
				node = getEAINodeFromTable(tmp_a,tmp_b);
				offset = getEAIActualOffset(tmp_a,tmp_b);

				if (node->_nodeType == NODE_Script) {
					struct Shader_Script * sp;

					/* we send along the script number, not the node pointer */
					sp = (struct Shader_Script *) (X3D_SCRIPT(node)->__scriptObj);

					/* some print statements here because this is a script node 
					printf ("ah! so noderef  %d, node %u is script num %d\n",tmp_a,node,sp->num);
					printf ("this is a script node in a REGLISTENER command! %d\n",tmp_b);
					*/

					node = offsetPointer_deref(struct X3D_Node *,0,sp->num);
					directionFlag = FROM_SCRIPT;
				}

				/* so, count = query id, tmp_a pointer, tmp_b, offset, ctmp[0] type, tmp_c, length*/
				ctmp[1]=0;

				if (eaiverbose) printf ("UNREGISTERLISTENER from %p foffset %d fieldlen %d type %s \n",
						node, offset ,tmp_c,ctmp);


				/* put the address of the listener area in a string format for registering
				   the route - the route propagation will copy data to here */
				/* set up the route from this variable to the handle Listener routine */
				CRoutes_Register  (0,node, offset, X3D_NODE(tg->EAIServ.EAIListenerData), 0, (int) tmp_c,(void *) 
					&EAIListener, directionFlag, (count<<8)+mapEAItypeToFieldType(ctmp[0])); /* encode id and type here*/

				sprintf (th->outBuffer,"RE\n%f\n%d\n0",TickTime(),count);
				break;
				}

			case GETVALUE: {
				handleEAIGetValue(command, &EAI_BUFFER_CUR,count);
				break;
				}
			case REPLACEWORLD:  {
				EAI_RW(&EAI_BUFFER_CUR);
				sprintf (th->outBuffer,"RE\n%f\n%d\n0",TickTime(),count);
				break;
				}

			case GETPROTODECL:  {
				sprintf (th->outBuffer,"RE\n%f\n%d\n%s",TickTime(),count,SAI_StrRetCommand ((char) command,&EAI_BUFFER_CUR));
				break;
				}
			case REMPROTODECL: 
			case UPDPROTODECL: 
			case UPDNAMEDNODE: 
			case REMNAMEDNODE:  {
				if (eaiverbose) {	
					printf ("SV int ret command ..%s\n",&EAI_BUFFER_CUR);
				}	
				sprintf (th->outBuffer,"RE\n%f\n%d\n%d",TickTime(),count,
					SAI_IntRetCommand ((char) command,&EAI_BUFFER_CUR));
				break;
				}
			case ADDROUTE:
			case DELETEROUTE:  {
				handleRoute (command, &EAI_BUFFER_CUR,count);
				break;
				}

		  	case STOPFREEWRL: {
				if (!RUNNINGASPLUGIN) {
					fwl_doQuit();
				    break;
				}
			    }
			  case VIEWPOINT: {
				/* do the viewpoints. Note the spaces in the strings */
				if (!strncmp(&EAI_BUFFER_CUR, " NEXT",5)) fwl_Next_ViewPoint();
				if (!strncmp(&EAI_BUFFER_CUR, " FIRST",6)) fwl_First_ViewPoint();
				if (!strncmp(&EAI_BUFFER_CUR, " LAST",5)) fwl_Last_ViewPoint();
				if (!strncmp(&EAI_BUFFER_CUR, " PREV",5)) fwl_Prev_ViewPoint();

				sprintf (th->outBuffer,"RE\n%f\n%d\n0",TickTime(),count);
				break;
			    }

			case LOADURL: {
				/* signal that we want to send the Anchor pass/fail to the EAI code */
				p->waiting_for_anchor = TRUE;

				/* make up the URL from what we currently know */
				createLoadURL(&EAI_BUFFER_CUR);


				/* prep the reply... */
				sprintf (th->outBuffer,"RE\n%f\n%d\n",TickTime(),count);

				/* now tell the fwl_RenderSceneUpdateScene that BrowserAction is requested... */
				setAnchorsAnchor( get_EAIEventsIn_AnchorNode()); //&tg->EAIEventsIn.EAI_AnchorNode;
				tg->RenderFuncs.BrowserAction = TRUE;
				break;
				}

			case CREATEPROTO: 
			case CREATENODE: {
				/* sanitize this string - remove leading and trailing garbage */
				rb = 0;
				while ((EAI_BUFFER_CUR!=0) && (EAI_BUFFER_CUR <= ' ')) bufPtr++;
				while (EAI_BUFFER_CUR > ' ') { ctmp[rb] = EAI_BUFFER_CUR; rb ++; bufPtr++; }

				ctmp[rb] = 0;
				if (eaiverbose) {	
					printf ("CREATENODE/PROTO %s\n",ctmp);
				}	

				/* set up the beginnings of the return string... */
				sprintf (th->outBuffer,"RE\n%f\n%d\n",TickTime(),count);
	
				retGroup = createNewX3DNode(NODE_Group);
				if (command == CREATENODE) {
					if (eaiverbose) {	
						printf ("CREATENODE, %s is this a simple node? %d\n",ctmp,findFieldInNODES(ctmp));
					}	

					ctype = findFieldInNODES(ctmp);
					if (ctype > -1) {
						/* yes, use C only to create this node */
						sprintf (ctmp, "%ld",(long int) createNewX3DNode(ctype));
						outBufferCat(ctmp);
						/* set ra to 0 so that the sprintf below is not used */
						ra = 0;
					} else {
						ra = EAI_CreateVrml("CREATENODE",ctmp,retGroup); 
					}
				} else if (command == CREATEPROTO)
					ra = EAI_CreateVrml("CREATEPROTO",ctmp,retGroup); 
				else 
					printf ("eai - huh????\n");


				for (rb = 0; rb < retGroup->children.n; rb++) {
					sprintf (ctmp,"%ld ", (long int) retGroup->children.p[rb]);
					outBufferCat(ctmp);
				}

				markForDispose(X3D_NODE(retGroup),FALSE);
				break;
				}

			case GETFIELDDEFS: {
				/* get a list of fields of this node */
				sscanf (&EAI_BUFFER_CUR,"%d",&ra);
				makeFIELDDEFret(ra,count);
				break;
				}

			case GETNODEDEFNAME: {
				/* return a def name for this node. */
				sprintf (th->outBuffer,"RE\n%f\n%d\n%s",TickTime(),count,
					SAI_StrRetCommand ((char) command,&EAI_BUFFER_CUR));

				break;
				}

			default: {
				printf ("unhandled command :%c: %d\n",command,command);
				outBufferCat( "unknown_EAI_command");
				break;
				}

			}

		if (command != SENDEVENT)  {
			if (command != LOADURL) outBufferCat("\nRE_EOT");
			EAI_send_string (th->outBuffer,tg->EAIServ.EAIlistenfd);
		}

		/* printf ("end of command, remainder %d ",strlen(&EAI_BUFFER_CUR)); */
		/* skip to the next command */
		while (EAI_BUFFER_CUR >= ' ') bufPtr++;

		/* skip any new lines that may be there */
		while ((EAI_BUFFER_CUR == 10) || (EAI_BUFFER_CUR == 13)) bufPtr++;
		/* printf ("and %d : indx %d thread %d\n",strlen(&EAI_BUFFER_CUR),bufPtr,pthread_self()); */
	}
}

static void handleGETROUTES (char *bufptr, int repno) {
	int numRoutes;
	int count;
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	int fromOffset;
	int toOffset;
	char  ctmp[200];
	struct tEAIHelpers* th = &gglobal()->EAIHelpers;
	
	sprintf (th->outBuffer,"RE\n%f\n%d\n",TickTime(),repno);

	numRoutes = getRoutesCount();

	if (numRoutes < 2) {
		outBufferCat("0");
		return;
	}

	/* tell how many routes there are */
	sprintf (ctmp,"%d ",numRoutes-2);
	outBufferCat(ctmp);

	/* remember, in the routing table, the first and last entres are invalid, so skip them */
	for (count = 1; count < (numRoutes-1); count++) {
		getSpecificRoute (count,&fromNode, &fromOffset, &toNode, &toOffset);

		sprintf (ctmp, "%p %s %p %s ",fromNode,
			findFIELDNAMESfromNodeOffset(fromNode,fromOffset),
			toNode,
			findFIELDNAMESfromNodeOffset(toNode,toOffset)
			);
		outBufferCat(ctmp);
		/* printf ("route %d is:%s:\n",count,ctmp); */
	}

	/* printf ("getRoutes returns %s\n",buf); */
}

static void handleGETNODE (char *bufptr, int repno) {
	int retint;
	char ctmp[200];
	int mystrlen;
	struct tEAIHelpers* th;
	int eaiverbose;
	ttglobal tg = gglobal();
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;
	th = &tg->EAIHelpers;
	/*format int seq# COMMAND    string nodename*/

	retint=sscanf (bufptr," %s",ctmp);
	mystrlen = (int) strlen(ctmp);

	if (eaiverbose) {	
		printf ("GETNODE %s\n",ctmp); 
	}	

	/* is this the SAI asking for the root node? */
	if (strcmp(ctmp,SYSTEMROOTNODE)) {
		sprintf (th->outBuffer,"RE\n%f\n%d\n%d",TickTime(),repno, EAI_GetNode(ctmp));
	} else {
		/* yep i this is a call for the rootNode */
		sprintf (th->outBuffer,"RE\n%f\n%d\n%d",TickTime(),repno, EAI_GetRootNode());
	}
	if (eaiverbose) {	
		printf ("GETNODE returns %s\n",th->outBuffer); 
	}	
}

#define IGNORE_IF_FABRICATED_INTERNAL_NAME \
	if (cptr!=NULL) if (strncmp(FABRICATED_DEF_HEADER,cptr,strlen(FABRICATED_DEF_HEADER))==0) { \
		/* printf ("ok, got FABRICATED_DEF_HEADER from %s, ignoring this one\n",cptr); */ \
		cptr = NULL; \
	}



/* get the actual node type, whether Group, IndexedFaceSet, etc, and its DEF name, if applicapable */
static void handleGETEAINODETYPE (char *bufptr, int repno) {
	int retint;
	int nodeHandle;
	struct X3D_Node * myNode;
	char *cptr;
	char *myNT;
	struct tEAIHelpers *th = &gglobal()->EAIHelpers;
	/*format int seq# COMMAND    string nodename*/

	retint=sscanf (bufptr," %d",&nodeHandle);
	myNode = getEAINodeFromTable(nodeHandle,-1);

	if (myNode == NULL) {
		printf ("Internal EAI error, node %d not found\n",nodeHandle);
		sprintf (th->outBuffer,"RE\n%f\n%d\n__UNDEFINED __UNDEFINED",TickTime(),repno);
		return;
	}
		
	/* so, this is a valid node, lets find out if it is DEFined or whatever... */
	/* Get the Node type. If it is a PROTO, get the proto def name, if not, just get the X3D node name */
	if ((myNode->_nodeType == NODE_Group) &&  (X3D_GROUP(myNode)->FreeWRL__protoDef != INT_ID_UNDEFINED)) {
		myNT = parser_getPROTONameFromNode(myNode);
		if (myNT == NULL) {
			myNT = "XML_PROTO"; /* add this if we need to parse XML proto getTypes */
		}
	} else {
		myNT = (char *) stringNodeType(myNode->_nodeType);
	}

        /* Try to get X3D node name */
	#ifdef IPHONE
	cptr = NULL; /* no xml parsing for now in iphone */
	#else
	cptr = X3DParser_getNameFromNode(myNode);
	#endif
	IGNORE_IF_FABRICATED_INTERNAL_NAME


	if (cptr != NULL) {
		sprintf (th->outBuffer,"RE\n%f\n%d\n%s %s",TickTime(),repno,myNT, cptr);
		return;
	}

        /* Try to get VRML node name */
	cptr= parser_getNameFromNode(myNode);
	IGNORE_IF_FABRICATED_INTERNAL_NAME
	if (cptr != NULL) {
		/* Only one of these is right ..... */
		/* I think it is the first one, because we would have had to know the DEF name in the first place. */
		/* sprintf (th->outBuffer,"RE\n%f\n%d\n\\"%s\"",TickTime(),repno,myNT); */
		   sprintf (th->outBuffer,"RE\n%f\n%d\n\"%s\" \"%s\"",TickTime(),repno,myNT, cptr);
		/* sprintf (th->outBuffer,"RE\n%f\n%d\n\"%s\"",TickTime(),repno, cptr); */
		return;
	}

	/* no, this node is just undefined */
	sprintf (th->outBuffer,"RE\n%f\n%d\n%s __UNDEFINED",TickTime(),repno,myNT);
}


/* add or delete a route */
static void handleRoute (char command, char *bufptr, int repno) {
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	char fieldTemp[2000];
	int fromOffset, toOffset;
	int adrem;
	int fromfieldType, fromfieldNode, fromretNode, fromretField, fromdataLen;
	int fromscripttype;
	int fromxxx;
	int tofieldNode, toretNode, toretField, todataLen, tofieldType;
	int toscripttype;
	int toxxx;
	char *x;
	int ftlen;

	int rv;
	int eaiverbose;
	struct tEAIHelpers *th = &gglobal()->EAIHelpers;
	eaiverbose = gglobal()->EAI_C_CommonFunctions.eaiverbose;
	/* assume that all is ok right now */
	rv = TRUE;

	/* get ready for the reply */
	sprintf (th->outBuffer,"RE\n%f\n%d\n",TickTime(),repno);

	if (eaiverbose) printf ("handleRoute, string %s\n",bufptr);
	
	/* ------- worry about the route from section -------- */

	/* read in the fromNode pointer */
	while (*bufptr == ' ') bufptr++;

	/* get the from Node */
        sscanf(bufptr, "%u", (unsigned int *)&fromfieldNode);

        /* copy the from field into the "fieldTemp" array */
        x = fieldTemp; ftlen = 0;

	/* skip to the beginning of the field name */
        while (*bufptr != ' ') bufptr++; while (*bufptr == ' ') bufptr++;

	/* copy the field over */
        while ((*bufptr > ' ') && (ftlen <1000)) { *x = *bufptr;      x++; bufptr++; ftlen++;}
        *x = '\0';

	/* and, get the info for this one */
	EAI_GetType (fromfieldNode, fieldTemp, "outputOnly", &fromretNode, &fromretField, &fromdataLen, &fromfieldType, &fromscripttype, &fromxxx);


	/* skip past the first field, to get ready for the next one */
	while (*bufptr != ' ') bufptr++; while (*bufptr == ' ') bufptr++;

	/* ------- now, the route to section -------- */
	/* get the to Node */

        sscanf(bufptr, "%u", (unsigned int *)&tofieldNode);

        /* copy the to field into the "fieldTemp" array */
        x = fieldTemp; ftlen = 0;

	/* skip to the beginning of the field name */
        while (*bufptr != ' ') bufptr++; while (*bufptr == ' ') bufptr++;

	/* copy the field over */
        while ((*bufptr > ' ') && (ftlen <1000)) { *x = *bufptr;      x++; bufptr++; ftlen++;}
        *x = '\0';

	/* and, get the info for this one */
	EAI_GetType (tofieldNode, fieldTemp, "inputOnly", &toretNode, &toretField, &todataLen, &tofieldType, &toscripttype, &toxxx);

	if (eaiverbose) printf ("so, we are routing from %d:%d to %d:%d, fieldtypes %d:%d, datalen %d:%d\n",
		fromretNode, toretNode, fromretField,
		toretField, fromfieldType,tofieldType, fromdataLen,todataLen);

	/* are both fieldtypes the same, and are both valid? */
	rv= ((fromfieldType==tofieldType) &&(fromfieldType != -1));

	/* ------- if we are ok, call the routing code  -------- */
	if (rv) {
		if (command == ADDROUTE) adrem = 1;
		else adrem = 0;

		/* get the C node and field offset for the nodes now */
		fromNode = (struct X3D_Node*) getEAINodeFromTable(fromretNode,fromretField);
		toNode = (struct X3D_Node*) getEAINodeFromTable(toretNode,toretField);
		fromOffset = getEAIActualOffset((int)fromretNode,fromretField);
		toOffset = getEAIActualOffset(toretNode,toretField);

		/* is this an add or delete route? */
		if (command == ADDROUTE) CRoutes_RegisterSimple(fromNode,fromOffset,toNode,toOffset,fromfieldType);
		else CRoutes_RemoveSimple(fromNode,fromOffset,toNode,toOffset,fromfieldType);

		outBufferCat( "0");
	} else {
		outBufferCat( "1");
	}
}

/* for a GetFieldTypes command for a node, we return a string giving the field types */

static void makeFIELDDEFret(int myptr, int repno) {
	struct X3D_Node *boxptr;
	int myc;
	int *np;
	char myline[200];
	/* Used for heavy tracing with eaiverbose */
	char *tmpptr;
	int  dtmp;
	char ctmp;
	char utilBuf[EAIREADSIZE];
	int errcount;
	ttglobal tg = gglobal();
	int eaiverbose;
	struct tEAIHelpers *th = &tg->EAIHelpers;
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;

	memset(utilBuf,'\0',sizeof(utilBuf));

	boxptr = getEAINodeFromTable(myptr,-1);

	if (eaiverbose) {
		printf ("GETFIELDDEFS, node %u -> %p\n",(unsigned int)myptr, boxptr);
	}

	if (boxptr == 0) {
		printf ("makeFIELDDEFret have null node here \n");
		sprintf (th->outBuffer,"RE\n%f\n%d\n0",TickTime(),repno);
		return;
	}

	printf ("node type is %s\n",stringNodeType(boxptr->_nodeType));
	
	/* Iterate over all the fields in the node */
	np = (int *) NODE_OFFSETS[boxptr->_nodeType];
	myc = 0;
	while (*np != -1) {
		/* is this a hidden field? */
		if (0 != strncmp(stringFieldType(np[0]), "_", 1) ) {
			if (eaiverbose) {
				ctmp = (char) mapFieldTypeToEAItype(np[2]) ;
				dtmp = mapEAItypeToFieldType(ctmp) ;

				tmpptr = offsetPointer_deref (char *, boxptr,np[1]);
				printf("%s,%d ",__FILE__,__LINE__) ;
				printf("Field %d %s , ", myc, stringFieldType(np[0])) ;
				printf("offset=%d bytes , ", np[1]) ;

				printf("field_type= %c (%d) , ", ctmp , dtmp) ;
				printf("Routing=%s , ", stringKeywordType(np[3])) ;
				printf("Spec=%d , ", np[4]) ;	

				errcount = UtilEAI_Convert_mem_to_ASCII (dtmp,tmpptr, utilBuf);
				if (0 == errcount) {
					printf ("\t\tValue = %s\n",utilBuf);
				} else {
					printf ("\t\tValue = indeterminate....\n");
				}
			}
			myc ++; 
		}
		np +=5;
	}

	sprintf (th->outBuffer,"RE\n%f\n%d\n",TickTime(),repno);

/* AFAIK Mon May 10 21:04:48 BST 2010 The EAI no longer passes a count, nor array markers ([]) */
/*
	sprintf (myline, "%d [",myc);
	outBufferCat( myline);
*/

	/* now go through and get the name, type, keyword */
	np = (int *) NODE_OFFSETS[boxptr->_nodeType];
	while (*np != -1) {
		/* if (strcmp (FIELDNAMES[*np],"_") != 0) { */
		if (0 != strncmp(stringFieldType(np[0]), "_", 1) ) {
			/*
			sprintf (myline,"%s %c %s ",stringFieldType(np[0]), (char) mapFieldTypeToEAItype(np[2]), 
				stringKeywordType(np[3]));
			*/
			sprintf (myline,"\"%s\" ",stringFieldType(np[0])) ;
			outBufferCat( myline);
		}
		np += 5;
	}
/*
	sprintf (myline, "]");
	outBufferCat( myline);
*/
}


#endif //EXCLUDE_EAI



/* EAI, replaceWorld. */
void EAI_RW(char *str) {
	struct X3D_Node *newNode;
	int i;

	/* clean the slate! keep EAI running, though */
	printf("EAI replace world, calling kill_oldWorld\n");
	kill_oldWorld(FALSE,TRUE,__FILE__,__LINE__);

	/* go through the string, and send the nodes into the rootnode */
	/* first, remove the command, and get to the beginning of node */
	while ((*str != ' ') && (strlen(str) > 0)) str++;
	while (isspace(*str)) str++;
	while (strlen(str) > 0) {
		i = sscanf (str, "%u",(unsigned int *)&newNode);

		if (i>0) {
			AddRemoveChildren (X3D_NODE(rootNode()),offsetPointer_deref(void*,rootNode(),offsetof (struct X3D_Group, children)),&newNode,1,1,__FILE__,__LINE__);
		}
		while (isdigit(*str)) str++;
		while (isspace(*str)) str++;
	}
}

#if !defined(EXCLUDE_EAI)

void createLoadURL(char *bufptr) {
	#define strbrk " :loadURLStringBreak:"
	int count;
	char *spbrk;
	int retint;		/* used to get retval from sscanf */
	//struct tEAIEventsIn *t = &gglobal()->EAIEventsIn;
	ppEAIEventsIn p = (ppEAIEventsIn)gglobal()->EAIEventsIn.prv;

	/* fill in Anchor parameters */
	p->EAI_AnchorNode.description = newASCIIString("From EAI");

	/* fill in length fields from string */
	while (*bufptr==' ') bufptr++;
	retint=sscanf (bufptr,"%d",&p->EAI_AnchorNode.url.n);
	while (*bufptr>' ') bufptr++;
	while (*bufptr==' ') bufptr++;
	retint=sscanf (bufptr,"%d",&p->EAI_AnchorNode.parameter.n);
	while (*bufptr>' ') bufptr++;
	while (*bufptr==' ') bufptr++;

	/* now, we should be at the strings. */
	bufptr--;

	/* MALLOC the sizes required */
	if (p->EAI_AnchorNode.url.n > 0) p->EAI_AnchorNode.url.p = MALLOC(struct Uni_String **, p->EAI_AnchorNode.url.n * sizeof (struct Uni_String));
	if (p->EAI_AnchorNode.parameter.n > 0) p->EAI_AnchorNode.parameter.p = MALLOC(struct Uni_String **, p->EAI_AnchorNode.parameter.n * sizeof (struct Uni_String));

	for (count=0; count<p->EAI_AnchorNode.url.n; count++) {
		bufptr += strlen(strbrk);
		/* printf ("scanning, at :%s:\n",bufptr); */
		
		/* nullify the next "strbrk" */
		spbrk = strstr(bufptr,strbrk);
		if (spbrk!=NULL) *spbrk='\0';

		p->EAI_AnchorNode.url.p[count] = newASCIIString(bufptr);

		if (spbrk!=NULL) bufptr = spbrk;
	}
	for (count=0; count<p->EAI_AnchorNode.parameter.n; count++) {
		bufptr += strlen(strbrk);
		/* printf ("scanning, at :%s:\n",bufptr); */
		
		/* nullify the next "strbrk" */
		spbrk = strstr(bufptr,strbrk);
		if (spbrk!=NULL) *spbrk='\0';

		p->EAI_AnchorNode.parameter.p[count] = newASCIIString(bufptr);

		if (spbrk!=NULL) bufptr = spbrk;
	}
	/* EAI_AnchorNode.__parenturl = newASCIIString("./"); */
}



/* if we have a LOADURL command (loadURL in java-speak) we call Anchor code to do this.
   here we tell the EAI code of the success/fail of an anchor call, IF the EAI is 
   expecting such a call */

void EAI_Anchor_Response (int resp) {
	char myline[1000];
	ppEAIEventsIn p;
	//ppEAIServ ps;
	ttglobal tg = gglobal();
	p = (ppEAIEventsIn)tg->EAIEventsIn.prv;
	//ps = (ppEAIServ)tg->EAIServ.prv;
	if (p->waiting_for_anchor) {
		if (resp) strcpy (myline,"OK\nRE_EOT");
		else strcpy (myline,"FAIL\nRE_EOT");
		EAI_send_string (myline,tg->EAIServ.EAIlistenfd);
	}
	p->waiting_for_anchor = FALSE;
}

#endif //EXCLUDE_EAI
