/*
=INSERT_TEMPLATE_HERE=

$Id: EAIEventsIn.c,v 1.24 2009/05/18 19:05:44 crc_canada Exp $

Handle incoming EAI (and java class) events with panache.

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h" 
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../vrml_parser/CProto.h"
#include "../vrml_parser/CParse.h"
#include "../world_script/CScripts.h"

#include "../input/EAIheaders.h"
#include "../world_script/fieldSet.h"
#include "../scenegraph/Viewer.h"
#include "../opengl/Textures.h"
#include "../x3d_parser/X3DParser.h"

#include "EAIHelpers.h"

#include <ctype.h> /* FIXME: config armor */


#define EAI_BUFFER_CUR EAIbuffer[bufPtr]

/* used for loadURL */
struct X3D_Anchor EAI_AnchorNode;
int waiting_for_anchor = FALSE;

void createLoadURL(char *bufptr);
void makeFIELDDEFret(uintptr_t,char *buf,int c);
void handleRoute (char command, char *bufptr, char *buf, int repno);
void handleGETNODE (char *bufptr, char *buf, int repno);
void handleGETROUTES (char *bufptr, char *buf, int repno);
void handleGETEAINODETYPE (char *bufptr, char *buf, int repno);


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


void EAI_parse_commands () {
	char buf[EAIREADSIZE];	/* return value place*/
	char ctmp[EAIREADSIZE];	/* temporary character buffer*/
	char dtmp[EAIREADSIZE];	/* temporary character buffer*/
	uintptr_t nodarr[200]; /* returning node/backnode combos from CreateVRML fns.*/

	int count;
	char command;
	int perlNode; uintptr_t cNode;
	int bufPtr = 0;		/* where we are in the EAI input buffer */
	
	uintptr_t ra,rb,rc,rd;	/* temps*/
	int tmp_a, tmp_b, tmp_c;

	unsigned int scripttype;
	char *EOT;		/* ptr to End of Text marker*/
	int retint;		/* used for getting retval for sscanf */

	struct X3D_Node *boxptr;
        int ctype;
	int xxx;

	while (EAI_BUFFER_CUR> 0) {
		if (eaiverbose) {
			printf ("EAI_parse_commands:start of while loop, strlen %d str :%s:\n",strlen((&EAI_BUFFER_CUR)),(&EAI_BUFFER_CUR));
		}

		/* step 1, get the command sequence number */
		if (sscanf ((&EAI_BUFFER_CUR),"%d",&count) != 1) {
			printf ("EAI_parse_commands, expected a sequence number on command :%s:\n",(&EAI_BUFFER_CUR));
			count = 0;
		}
		if (eaiverbose) {
			printf ("EAI - seq number %d\n",count);
		}

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
			printf ("EAI command %s (%c) strlen %d\n",eaiPrintCommand(command), command,strlen(&EAI_BUFFER_CUR));

		bufPtr++;

		/* return is something like: $hand->print("RE\n$reqid\n1\n$id\n");*/
		if (eaiverbose) {
			printf ("\n... %d ",count);
		}

		switch (command) {
			case GETRENDPROP: {
				/* is MultiTexture initialized yet? */
				if (maxTexelUnits < 0) init_multitexture_handling();

				sprintf (buf,"RE\n%f\n%d\n%s %dx%d %d %s %d %f",TickTime,count,
					"SMOOTH",				/* Shading */
					global_texSize, global_texSize, 	/* Texture size */	
					maxTexelUnits,				/* texture units */
					"FALSE",				/* antialiased? */
					displayDepth,				/* bit depth of display */
					256.0					/* amount of memory left on card -
										   can not find this in OpenGL, so
										   just make it large... */
					);
				break;
				}

			case GETNAME: {
				sprintf (buf,"RE\n%f\n%d\n%s",TickTime,count,BrowserName);
				break;
				}
			case GETVERSION: {
				sprintf (buf,"RE\n%f\n%d\n%s",TickTime,count,libFreeWRL_get_version());
				break;
				}
			case GETENCODING: {
				sprintf (buf,"RE\n%f\n%d\n%d",TickTime,count,currentFileVersion);
				break;
				}
			case GETCURSPEED: {
				/* get the BrowserSpeed variable updated */
				getCurrentSpeed();
				sprintf (buf,"RE\n%f\n%d\n%f",TickTime,count,BrowserSpeed);
				break;
				}
			case GETFRAMERATE: {
				sprintf (buf,"RE\n%f\n%d\n%f",TickTime,count,BrowserFPS);
				break;
				}
			case GETURL: {
				sprintf (buf,"RE\n%f\n%d\n%s",TickTime,count,BrowserFullPath);
				break;
				}
			case GETNODE:  {
				handleGETNODE(&EAI_BUFFER_CUR,buf,count);
				break;
			}
			case GETROUTES:  {
				handleGETROUTES(&EAI_BUFFER_CUR,buf,count);
				break;
			}

			case GETEAINODETYPE: {
				handleGETEAINODETYPE(&EAI_BUFFER_CUR,buf,count);
				break;
			}

			case GETNODETYPE: {
				retint = sscanf(&EAI_BUFFER_CUR,"%d",&cNode);
				if (cNode != 0) {
					boxptr = X3D_NODE(cNode);
					sprintf (buf,"RE\n%f\n%d\n%d",TickTime,count,getSAI_X3DNodeType (
						boxptr->_nodeType));
				} else {
					sprintf (buf,"RE\n%f\n%d\n-1",TickTime,count);
				}
				/* printf ("GETNODETYPE, for node %s, returns %s\n",(&EAI_BUFFER_CUR),buf); */
					
				break;
				}
			case GETFIELDTYPE:  {
				/*format int seq# COMMAND  int node#   string fieldname   string direction*/

				retint=sscanf (&EAI_BUFFER_CUR,"%d %d %s %s",&perlNode, &cNode, ctmp,dtmp);
				if (eaiverbose) {	
					printf ("GETFIELDTYPE cptr %d %s %s\n",cNode, ctmp, dtmp);
				}	

				EAI_GetType (cNode, ctmp, dtmp, &ra, &rb, &rc, &rd, &scripttype, &xxx);

				sprintf (buf,"RE\n%f\n%d\n%d %d %d %c %d %s",TickTime,count,ra,rb,rc,rd,
						scripttype,stringKeywordType(xxx));
				break;
				}
			case SENDEVENT:   {
				/*format int seq# COMMAND NODETYPE pointer offset data*/
				setField_FromEAI (&EAI_BUFFER_CUR);
				if (eaiverbose) {	
					printf ("after SENDEVENT, strlen %d\n",strlen(&EAI_BUFFER_CUR));
				}
				break;
				}
			case MIDIINFO: {
				EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");
				/* if we do not have a string yet, we have to do this...*/
				while (EOT == NULL) {
					EAIbuffer = read_EAI_socket(EAIbuffer,&EAIbufcount, &EAIbufsize, &EAIlistenfd);
					EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");
				}

				*EOT = 0; /* take off the EOT marker*/
				ReWireRegisterMIDI(&EAI_BUFFER_CUR);

				/* finish this for now - note the pointer math. */
				bufPtr = EOT+3-EAIbuffer;
				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}
			case MIDICONTROL: {
				/* sprintf (buf,"RE\n%f\n%d\n%d",TickTime,count, ReWireMIDIControl(&EAI_BUFFER_CUR)); */
				ReWireMIDIControl(&EAI_BUFFER_CUR);
				break;
				}
			case CREATEVU:
			case CREATEVS: {
				/*format int seq# COMMAND vrml text     string EOT*/
				if (command == CREATEVS) {
					if (eaiverbose) {	
						printf ("CREATEVS %s\n",&EAI_BUFFER_CUR);
					}	

					EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");
					/* if we do not have a string yet, we have to do this...*/
					while (EOT == NULL) {
						EAIbuffer = read_EAI_socket(EAIbuffer,&EAIbufcount, &EAIbufsize, &EAIlistenfd);
						EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");
					}

					*EOT = 0; /* take off the EOT marker*/

					ra = EAI_CreateVrml("String",(&EAI_BUFFER_CUR),nodarr,200);
					/* finish this, note the pointer maths */
					bufPtr = EOT+3-EAIbuffer;
				} else {
 					char *filename = (char *)MALLOC(1000);
					char *mypath;

					/* sanitize this string - remove leading and trailing garbage */
					rb = 0;
					while ((EAI_BUFFER_CUR!=0) && (EAI_BUFFER_CUR <= ' ')) bufPtr++;
					while (EAI_BUFFER_CUR > ' ') { ctmp[rb] = EAI_BUFFER_CUR; rb ++; bufPtr++; }

					/* ok, lets make a real name from this; maybe it is local to us? */
					ctmp[rb] = 0;

					/* get the current parent */
					mypath = STRDUP(ctmp);
					/* printf ("CREATEVU, mypath %s\n",mypath); */

					/* and strip off the file name, leaving any path */
					removeFilenameFromPath (mypath);
					/* printf ("CREATEVU, mypath sans file: %s\n",mypath); */

					/* add the two together */
					makeAbsoluteFileName(filename,mypath,ctmp);
					/* printf ("CREATEVU, filename, %s\n",filename); */

					if (eaiverbose) {	
						printf ("CREATEVU %s\n",filename);
					}	
					ra = EAI_CreateVrml("URL",filename,nodarr,200);
					FREE_IF_NZ(filename);
					FREE_IF_NZ(mypath);
				}

				sprintf (buf,"RE\n%f\n%d\n",TickTime,count);
				for (rb = 0; rb < ra; rb++) {
					/* printf ("create returns %d of %d %u\n",rb, ra, nodarr[rb]); */
					/* we have to skip the "perl" nodes, as they are not used and are
					   always going to be zero, but the real ones have to be EAIregistered */
					if (nodarr[rb] == 0)
						strcat (buf, "0 ");
					else {
						sprintf (ctmp,"%d ", registerEAINodeForAccess(X3D_NODE(nodarr[rb])));
						strcat (buf,ctmp);
					}
				}
				break;
				}

			case SENDCHILD :  {
				struct X3D_Node *node;
				uintptr_t *address;

				/*format int seq# COMMAND  int node#   ParentNode field ChildNode*/

				retint=sscanf (&EAI_BUFFER_CUR,"%d %d %s %d",&ra,&rb,ctmp,&rc);

				node = getEAINodeFromTable(ra,rb);
				address = getEAIMemoryPointer (ra,rb);
				sprintf (dtmp,"%u",getEAINodeFromTable(rc,-1));

				if (eaiverbose) {	
					printf ("SENDCHILD Parent: %d ParentField: %d %s Child: %s\n",ra, rb, ctmp, dtmp);
				}	

				/* we actually let the end of eventloop code determine whether this is an add or
				   remove, or whatever - it will handle the addChildren and removeChildren well
				   so the add/remove/replace parameter of getMFNodetype is always 1 here */

				getMFNodetype (dtmp,(struct Multi_Node *)address, node, 1);

				/* tell the routing table that this node is updated - used for RegisterListeners */
				MARK_EVENT(node,getEAIActualOffset(ra,rb));

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}
			case REGLISTENER: {
				struct X3D_Node * node;
				int offset;
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

					node = (uintptr_t) sp->num;
					directionFlag = FROM_SCRIPT;
				}

				/* so, count = query id, tmp_a pointer, tmp_b, offset, ctmp[0] type, tmp_c, length*/
				ctmp[1]=0;

				if (eaiverbose) printf ("REGISTERLISTENER from %lu foffset %d fieldlen %d type %s \n",
						(uintptr_t)node, offset ,tmp_c,ctmp);


				/* put the address of the listener area in a string format for registering
				   the route - the route propagation will copy data to here */
				sprintf (EAIListenerArea,"%lu:0",(uintptr_t)&EAIListenerData);

				/* set up the route from this variable to the handle_Listener routine */
				CRoutes_Register  (1,node, offset, 1, EAIListenerArea, (int) tmp_c,(void *) 
					&handle_Listener, directionFlag, (count<<8)+mapEAItypeToFieldType(ctmp[0])); /* encode id and type here*/
				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
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

					node = (uintptr_t) sp->num;
					directionFlag = FROM_SCRIPT;
				}

				/* so, count = query id, tmp_a pointer, tmp_b, offset, ctmp[0] type, tmp_c, length*/
				ctmp[1]=0;

				if (eaiverbose) printf ("UNREGISTERLISTENER from %lu foffset %d fieldlen %d type %s \n",
						(uintptr_t)node, offset ,tmp_c,ctmp);


				/* put the address of the listener area in a string format for registering
				   the route - the route propagation will copy data to here */
				sprintf (EAIListenerArea,"%lu:0",(uintptr_t)&EAIListenerData);

				/* set up the route from this variable to the handle_Listener routine */
				CRoutes_Register  (0,node, offset, 1, EAIListenerArea, (int) tmp_c,(void *) 
					&handle_Listener, directionFlag, (count<<8)+mapEAItypeToFieldType(ctmp[0])); /* encode id and type here*/

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}

			case GETVALUE: {
				handleEAIGetValue(command, &EAI_BUFFER_CUR,buf,count);
				break;
				}
			case REPLACEWORLD:  {
				EAI_RW(&EAI_BUFFER_CUR);
				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
				}

			case GETPROTODECL:  {
				sprintf (buf,"RE\n%f\n%d\n%s",TickTime,count,SAI_StrRetCommand ((char) command,&EAI_BUFFER_CUR));
				break;
				}
			case REMPROTODECL: 
			case UPDPROTODECL: 
			case UPDNAMEDNODE: 
			case REMNAMEDNODE:  {
				if (eaiverbose) {	
					printf ("SV int ret command ..%s\n",&EAI_BUFFER_CUR);
				}	
				sprintf (buf,"RE\n%f\n%d\n%d",TickTime,count,
					SAI_IntRetCommand ((char) command,&EAI_BUFFER_CUR));
				break;
				}
			case ADDROUTE:
			case DELETEROUTE:  {
				handleRoute (command, &EAI_BUFFER_CUR,buf,count);
				break;
				}

		  	case STOPFREEWRL: {
				if (!RUNNINGASPLUGIN) {
					doQuit();
				    break;
				}
			    }
			  case VIEWPOINT: {
				/* do the viewpoints. Note the spaces in the strings */
				if (!strcmp(&EAI_BUFFER_CUR, " NEXT")) Next_ViewPoint();
				if (!strcmp(&EAI_BUFFER_CUR, " FIRST")) First_ViewPoint();
				if (!strcmp(&EAI_BUFFER_CUR, " LAST")) Last_ViewPoint();
				if (!strcmp(&EAI_BUFFER_CUR, " PREV")) Prev_ViewPoint();

				sprintf (buf,"RE\n%f\n%d\n0",TickTime,count);
				break;
			    }

			case LOADURL: {
				/* signal that we want to send the Anchor pass/fail to the EAI code */
				waiting_for_anchor = TRUE;

				/* make up the URL from what we currently know */
				createLoadURL(&EAI_BUFFER_CUR);


				/* prep the reply... */
				sprintf (buf,"RE\n%f\n%d\n",TickTime,count);

				/* now tell the EventLoop that BrowserAction is requested... */
				AnchorsAnchor = &EAI_AnchorNode;
				BrowserAction = TRUE;
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
				sprintf (buf,"RE\n%f\n%d\n",TickTime,count);

				if (command == CREATENODE) {
					if (eaiverbose) {	
						printf ("CREATENODE, %s is this a simple node? %d\n",ctmp,findFieldInNODES(ctmp));
					}	

					ctype = findFieldInNODES(ctmp);
					if (ctype > -1) {
						/* yes, use C only to create this node */
						sprintf (ctmp, "0 %d ",createNewX3DNode(ctype));
						strcat (buf,ctmp);
						/* set ra to 0 so that the sprintf below is not used */
						ra = 0;
					} else {
						ra = EAI_CreateVrml("CREATENODE",ctmp,nodarr,200); 
					}
				} else if (command == CREATEPROTO)
					ra = EAI_CreateVrml("CREATEPROTO",ctmp,nodarr,200); 
				else 
					printf ("eai - huh????\n");


				for (rb = 0; rb < ra; rb++) {
					sprintf (ctmp,"%d ", nodarr[rb]);
					strcat (buf,ctmp);
				}
				break;
				}

			case GETFIELDDEFS: {
				/* get a list of fields of this node */
				sscanf (&EAI_BUFFER_CUR,"%d",&ra);
				makeFIELDDEFret(ra,buf,count);
				break;
				}

			case GETNODEDEFNAME: {
				/* return a def name for this node. */
				sprintf (buf,"RE\n%f\n%d\n%s",TickTime,count,
					SAI_StrRetCommand ((char) command,&EAI_BUFFER_CUR));

				break;
				}

			default: {
				printf ("unhandled command :%c: %d\n",command,command);
				strcat (buf, "unknown_EAI_command");
				break;
				}

			}

		/* send the response - events don't send a reply */
		/* and, Anchors send a different reply (loadURLS) */
		if ((command != SENDEVENT) && (command != MIDICONTROL)) {
			if (command != LOADURL) strcat (buf,"\nRE_EOT");
			if (command != MIDIINFO)
				EAI_send_string (buf,EAIlistenfd);
			else
				EAI_send_string(buf, EAIMIDIlistenfd);
		}

		/* printf ("end of command, remainder %d ",strlen(&EAI_BUFFER_CUR)); */
		/* skip to the next command */
		while (EAI_BUFFER_CUR >= ' ') bufPtr++;

		/* skip any new lines that may be there */
		while ((EAI_BUFFER_CUR == 10) || (EAI_BUFFER_CUR == 13)) bufPtr++;
		/* printf ("and %d : indx %d thread %d\n",strlen(&EAI_BUFFER_CUR),bufPtr,pthread_self()); */
	}
}

void handleGETROUTES (char *bufptr, char *buf, int repno) {
	int numRoutes;
	int count;
	uintptr_t fromNode;
	uintptr_t toNode;
	int fromOffset;
	int toOffset;
	char  ctmp[200];
	
	sprintf (buf,"RE\n%f\n%d\n",TickTime,repno);

	numRoutes = getRoutesCount();

	if (numRoutes < 2) {
		strcat (buf,"0");
		return;
	}

	/* tell how many routes there are */
	sprintf (ctmp,"%d ",numRoutes-2);
	strcat (buf,ctmp);

	/* remember, in the routing table, the first and last entres are invalid, so skip them */
	for (count = 1; count < (numRoutes-1); count++) {
		getSpecificRoute (count,&fromNode, &fromOffset, &toNode, &toOffset);

		sprintf (ctmp, "%d %d %s %d %d %s ",0,fromNode,
			findFIELDNAMESfromNodeOffset(X3D_NODE(fromNode),fromOffset),
			0,toNode,
			findFIELDNAMESfromNodeOffset(X3D_NODE(toNode),toOffset)
			);
		strcat (buf,ctmp);
		/* printf ("route %d is:%s:\n",count,ctmp); */
	}

	/* printf ("getRoutes returns %s\n",buf); */
}

void handleGETNODE (char *bufptr, char *buf, int repno) {
	int retint;
	char ctmp[200];
	int mystrlen;

	/*format int seq# COMMAND    string nodename*/

	retint=sscanf (bufptr," %s",ctmp);
	mystrlen = strlen(ctmp);

	if (eaiverbose) {	
		printf ("GETNODE %s\n",ctmp); 
	}	

	/* is this the SAI asking for the root node? */
	if (strcmp(ctmp,SYSTEMROOTNODE)) {
		sprintf (buf,"RE\n%f\n%d\n0 %d",TickTime,repno, EAI_GetNode(ctmp));
	} else {
		/* yep i this is a call for the rootNode */
		sprintf (buf,"RE\n%f\n%d\n0 %d",TickTime,repno, EAI_GetRootNode());
	}
	if (eaiverbose) {	
		printf ("GETNODE returns %s\n",buf); 
	}	
}

#define IGNORE_IF_FABRICATED_INTERNAL_NAME \
	if (cptr!=NULL) if (strncmp(FABRICATED_DEF_HEADER,cptr,strlen(FABRICATED_DEF_HEADER))==0) { \
		/* printf ("ok, got FABRICATED_DEF_HEADER from %s, ignoring this one\n",cptr); */ \
		cptr = NULL; \
	}



/* get the actual node type, whether Group, IndexedFaceSet, etc, and its DEF name, if applicapable */
void handleGETEAINODETYPE (char *bufptr, char *buf, int repno) {
	int retint;
	int nodeHandle;
	struct X3D_Node * myNode;
	char *cptr;
	char *myNT;

	/*format int seq# COMMAND    string nodename*/

	retint=sscanf (bufptr," %d",&nodeHandle);
	myNode = getEAINodeFromTable(nodeHandle,-1);

	if (myNode == NULL) {
		printf ("Internal EAI error, node %d not found\n",nodeHandle);
		sprintf (buf,"RE\n%f\n%d\n__UNDEFINED __UNDEFINED",TickTime,repno);
		return;
	}
		
	/* so, this is a valid node, lets find out if it is DEFined or whatever... */
	/* Get the Node type. If it is a PROTO, get the proto def name, if not, just get the X3D node name */
	if ((myNode->_nodeType == NODE_Group) &&  (X3D_GROUP(myNode)->FreeWRL__protoDef != 0)) {
		myNT = parser_getPROTONameFromNode(myNode);
		if (myNT == NULL) {
			myNT = "XML_PROTO"; /* add this if we need to parse XML proto getTypes */
		}
	} else {
		myNT = stringNodeType(myNode->_nodeType);
	}

        /* Try to get X3D node name */
	cptr = X3DParser_getNameFromNode(myNode);
	IGNORE_IF_FABRICATED_INTERNAL_NAME


	if (cptr != NULL) {
		sprintf (buf,"RE\n%f\n%d\n%s %s",TickTime,repno,myNT, cptr);
		return;
	}

        /* Try to get VRML node name */
	cptr= parser_getNameFromNode(myNode);
	IGNORE_IF_FABRICATED_INTERNAL_NAME
	if (cptr != NULL) {
		sprintf (buf,"RE\n%f\n%d\n%s %s",TickTime,repno,myNT, cptr);
		return;
	}

	/* no, this node is just undefined */
	sprintf (buf,"RE\n%f\n%d\n%s __UNDEFINED",TickTime,repno,myNT);
}


/* add or delete a route */
void handleRoute (char command, char *bufptr, char *buf, int repno) {
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	char fieldTemp[2000];
	int fromOffset, toOffset;
	int adrem;
	int fromfieldNode, fromretNode, fromretField, fromdataLen, fromfieldType, fromscripttype, fromxxx;
	int tofieldNode, toretNode, toretField, todataLen, tofieldType, toscripttype, toxxx;
	char *x;
	int ftlen;

	int rv;

	/* assume that all is ok right now */
	rv = TRUE;

	/* get ready for the reply */
	sprintf (buf,"RE\n%f\n%d\n",TickTime,repno);

	if (eaiverbose) printf ("handleRoute, string %s\n",bufptr);
	
	/* ------- worry about the route from section -------- */

	/* read in the fromNode pointer */
	while (*bufptr == ' ') bufptr++;

	/* get the from Node */
        sscanf(bufptr, "%d", &fromfieldNode);

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

        sscanf(bufptr, "%d", &tofieldNode);

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
		fromretNode,toretNode, fromretField,toretField,fromfieldType,tofieldType,fromdataLen,todataLen);

	/* are both fieldtypes the same, and are both valid? */
	rv= ((fromfieldType==tofieldType) &&(fromfieldType != -1));

	/* ------- if we are ok, call the routing code  -------- */
	if (rv) {
		int scriptFlags = 0;
		struct Shader_Script *sp;

		if (command == ADDROUTE) adrem = 1;
		else adrem = 0;

		/* get the C node and field offset for the nodes now */
		fromNode = (struct X3D_Node*) getEAINodeFromTable(fromretNode,fromretField);
		toNode = (struct X3D_Node*) getEAINodeFromTable(toretNode,toretField);
		fromOffset = getEAIActualOffset(fromretNode,fromretField);
		toOffset = getEAIActualOffset(toretNode,toretField);

		/* is this an add or delete route? */
		if (command == ADDROUTE) CRoutes_RegisterSimple(fromNode,fromOffset,toNode,toOffset,fromdataLen);
		else CRoutes_RemoveSimple(fromNode,fromOffset,toNode,toOffset,fromdataLen);

		strcat (buf, "0");
	} else {
		strcat (buf, "1");
	}
}


/* for a GetFieldTypes command for a node, we return a string giving the field types */

void makeFIELDDEFret(uintptr_t myptr, char *buf, int repno) {
	struct X3D_Node *boxptr;
	int myc;
	int *np;
	char myline[200];

	boxptr = X3D_NODE(myptr);
	
	/* printf ("GETFIELDDEFS, node %d\n",boxptr); */

	if (boxptr == 0) {
		printf ("makeFIELDDEFret have null node here \n");
		sprintf (buf,"RE\n%f\n%d\n0",TickTime,repno);
		return;
	}

	/* printf ("node type is %s\n",stringNodeType(boxptr->_nodeType)); */
	


	/* how many fields in this node? */
	np = (int *)NODE_OFFSETS[boxptr->_nodeType];
	myc = 0;
	while (*np != -1) {
		/* is this a hidden field? */
		if (strcmp (FIELDNAMES[*np],"_") != 0) {
			myc ++; 
		}

		np +=5;

	}

	sprintf (buf,"RE\n%f\n%d\n",TickTime,repno);

	sprintf (myline, "%d ",myc);
	strcat (buf, myline);

	/* now go through and get the name, type, keyword */
	np = (int *)NODE_OFFSETS[boxptr->_nodeType];
	while (*np != -1) {
		if (strcmp (FIELDNAMES[*np],"_") != 0) {
			sprintf (myline,"%s %c %s ",stringFieldType(np[0]), (char) mapFieldTypeToEAItype(np[2]), 
				stringKeywordType(np[3]));
			strcat (buf, myline);
		}
		np += 5;
	}
}



/* EAI, replaceWorld. */
void EAI_RW(char *str) {
	struct X3D_Node *newNode;
	int i;

	/* clean the slate! keep EAI running, though */
	kill_oldWorld(FALSE,TRUE,FALSE,__FILE__,__LINE__);

	/* go through the string, and send the nodes into the rootnode */
	/* first, remove the command, and get to the beginning of node */
	while ((*str != ' ') && (strlen(str) > 0)) str++;
	while (isspace(*str)) str++;
	while (strlen(str) > 0) {
		i = sscanf (str, "%u",&newNode);

		if (i>0) {
			AddRemoveChildren (rootNode,rootNode + offsetof (struct X3D_Group, children),&newNode,1,1,__FILE__,__LINE__);
		}
		while (isdigit(*str)) str++;
		while (isspace(*str)) str++;
	}
}


void createLoadURL(char *bufptr) {
	#define strbrk " :loadURLStringBreak:"
	int count;
	char *spbrk;
	int retint;		/* used to get retval from sscanf */


	/* fill in Anchor parameters */
	EAI_AnchorNode.description = newASCIIString("From EAI");

	/* fill in length fields from string */
	while (*bufptr==' ') bufptr++;
	retint=sscanf (bufptr,"%d",&EAI_AnchorNode.url.n);
	while (*bufptr>' ') bufptr++;
	while (*bufptr==' ') bufptr++;
	retint=sscanf (bufptr,"%d",&EAI_AnchorNode.parameter.n);
	while (*bufptr>' ') bufptr++;
	while (*bufptr==' ') bufptr++;

	/* now, we should be at the strings. */
	bufptr--;

	/* MALLOC the sizes required */
	if (EAI_AnchorNode.url.n > 0) EAI_AnchorNode.url.p = MALLOC(EAI_AnchorNode.url.n * sizeof (struct Uni_String));
	if (EAI_AnchorNode.parameter.n > 0) EAI_AnchorNode.parameter.p = MALLOC(EAI_AnchorNode.parameter.n * sizeof (struct Uni_String));

	for (count=0; count<EAI_AnchorNode.url.n; count++) {
		bufptr += strlen(strbrk);
		/* printf ("scanning, at :%s:\n",bufptr); */
		
		/* nullify the next "strbrk" */
		spbrk = strstr(bufptr,strbrk);
		if (spbrk!=NULL) *spbrk='\0';

		EAI_AnchorNode.url.p[count] = newASCIIString(bufptr);

		if (spbrk!=NULL) bufptr = spbrk;
	}
	for (count=0; count<EAI_AnchorNode.parameter.n; count++) {
		bufptr += strlen(strbrk);
		/* printf ("scanning, at :%s:\n",bufptr); */
		
		/* nullify the next "strbrk" */
		spbrk = strstr(bufptr,strbrk);
		if (spbrk!=NULL) *spbrk='\0';

		EAI_AnchorNode.parameter.p[count] = newASCIIString(bufptr);

		if (spbrk!=NULL) bufptr = spbrk;
	}
	EAI_AnchorNode.__parenturl = newASCIIString("./");
}



/* if we have a LOADURL command (loadURL in java-speak) we call Anchor code to do this.
   here we tell the EAI code of the success/fail of an anchor call, IF the EAI is 
   expecting such a call */

void EAI_Anchor_Response (int resp) {
	char myline[1000];
	if (waiting_for_anchor) {
		if (resp) strcpy (myline,"OK\nRE_EOT");
		else strcpy (myline,"FAIL\nRE_EOT");
		EAI_send_string (myline,EAIlistenfd);
	}
	waiting_for_anchor = FALSE;
}
