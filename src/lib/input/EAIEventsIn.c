/*
=INSERT_TEMPLATE_HERE=

$Id: EAIEventsIn.c,v 1.86 2012/07/08 15:17:45 dug9 Exp $

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


/************************************************************************/
/*									*/
/* Design notes:							*/
/*	FreeWRL is a server, the Java (or whatever) program is a client	*/
/*									*/
/*	Commands come in, and get answered to, except for sendEvents;	*/
/*	for these there is no response (makes system faster)		*/
/*									*/
/*	Nodes that are registered for listening to, send async		*/
/*	messages.							*/
/*									*/
/*	very simple example:						*/
/*		move a transform; Java code:				*/
/*									*/
/*		EventInMFNode addChildren;				*/
/*		EventInSFVec3f newpos;					*/
/*		try { root = browser.getNode("ROOT"); }			*/
/*		catch (InvalidNodeException e) { ... }			*/
/*									*/
/*		newpos=(EventInSFVec3f)root.getEventIn("translation");	*/
/*		val[0] = 1.0; val[1] = 1.0; val[2] = 1.0;		*/
/*		newpos.setValue(val);					*/
/*									*/
/*		Three EAI commands sent:				*/
/*			1) GetNode ROOT					*/
/*				returns a node identifier		*/
/*			2) GetType (nodeID) translation			*/
/*				returns posn in memory, length,		*/
/*				and data type				*/
/*									*/
/*			3) SendEvent posn-in-memory, len, data		*/
/*				returns nothing - assumed to work.	*/
/*									*/
/************************************************************************/

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
#include "EAIHeaders.h"

#include <ctype.h> /* FIXME: config armor */

#define EAI_BUFFER_CUR tg->EAICore.EAIbuffer[bufPtr]

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
* EAI_core_commands should only be called from
* fwl_EAI_handleBuffer(..) or
* fwl_MIDI_handleBuffer(..)
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

/*
 * This code used to sit in the socket server, but
 * the EAI buffer is no longer the same as the socket's buffer
 * The EAI buffer may still need sharing and locking...
 * Will leave that decision to the experts!!
 */
typedef struct pEAICore{
	pthread_mutex_t eaibufferlock;// = PTHREAD_MUTEX_INITIALIZER;
}* ppEAICore;

void *EAICore_constructor()
{
	void *v = malloc(sizeof(struct pEAICore));
	memset(v,0,sizeof(struct pEAICore));
	return v;
}
void EAICore_init(struct tEAICore* t){
	//private
	t->prv = EAICore_constructor();
	{
		ppEAICore p = (ppEAICore)t->prv;
		pthread_mutex_init(&(p->eaibufferlock), NULL);
	}
	//public
	t->EAIbufsize = EAIREADSIZE ;
}

struct X3D_Anchor* get_EAIEventsIn_AnchorNode()
{
	ppEAIEventsIn p = (ppEAIEventsIn)gglobal()->EAIEventsIn.prv;
	return (struct X3D_Anchor*)&p->EAI_AnchorNode;
}

#if !defined(EXCLUDE_EAI)
int fwl_EAI_allDone() {
	int eaiverbose;
	int bufPtr;
	int stillToDo;
	ttglobal tg = gglobal();
	bufPtr = tg->EAICore.EAIbufpos;
	stillToDo = (int)strlen((&EAI_BUFFER_CUR));
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;
	if (eaiverbose && stillToDo > 0) {
		printf ("EAI_allDone still to do: strlen %d str :%s:\n",stillToDo, (&EAI_BUFFER_CUR));
	}
	return stillToDo;
}

char * fwl_EAI_handleRest() {
	int eaiverbose;
	int bufPtr;
	int stillToDo;
	struct tEAIHelpers *th;
	ttglobal tg = gglobal();
	bufPtr = tg->EAICore.EAIbufpos;
	stillToDo = (int)strlen((&EAI_BUFFER_CUR));
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;

	if(NULL == tg->EAICore.EAIbuffer) {
		printf("fwl_EAI_handleRest() did not have a buffer, WTF!!") ;
		return NULL ;
	}
	if(stillToDo > 0) {
		if(eaiverbose) {
			printf("%s:%d fwl_EAI_handleRest: Buffer at %p , bufPtr=%d , still to do=%d str :%s:\n",\
			__FILE__,__LINE__,tg->EAICore.EAIbuffer,bufPtr,(int)strlen((&EAI_BUFFER_CUR)), (&EAI_BUFFER_CUR));
		}

		EAI_core_commands() ;

		th = &tg->EAIHelpers;
		return th->outBuffer ;
	} else {
		return "";
	}
}

char * fwl_EAI_handleBuffer(char *fromFront) {
	/* memcp from fromFront to &EAI_BUFFER_CUR */
	int eaiverbose;
	int len = strlen(fromFront) ;
	ttglobal tg = gglobal();
	struct tEAIHelpers *th;
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;

	if(NULL == tg->EAICore.EAIbuffer) {
		tg->EAICore.EAIbuffer = MALLOC(char *, tg->EAICore.EAIbufsize * sizeof (char));
		if(eaiverbose) {
			printf("fwl_EAI_handleBuffer() did not have a buffer, so create one at %p\n",tg->EAICore.EAIbuffer) ;
		}
	}
	if(eaiverbose) {
		printf("%s:%d fwl_EAI_handleBuffer: Buffer at %p is %d chars,",__FILE__,__LINE__,fromFront,len);
		printf("Copy to buffer at %p\n", tg->EAICore.EAIbuffer);
	}

	if(len <= EAIREADSIZE) {
		tg->EAICore.EAIbuffer[len] = '\0';
		memcpy(tg->EAICore.EAIbuffer, fromFront, len);

                //int EAIbufcount;                                /* pointer into buffer*/
                //int EAIbufpos;
		tg->EAICore.EAIbufpos = 0;
                tg->EAICore.EAIbufcount = 0;

		EAI_core_commands() ;

		th = &tg->EAIHelpers;
		return th->outBuffer ;
	} else {
		fwlio_RxTx_control(CHANNEL_EAI,RxTx_STOP) ;
		return "";
	}
}
char * fwl_MIDI_handleBuffer(char *fromFront) {
	/* memcp from fromFront to &EAI_BUFFER_CUR */
	int eaiverbose;
	int len = strlen(fromFront) ;
	ttglobal tg = gglobal();
	struct tEAIHelpers *th;
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;

	if(NULL == tg->EAICore.EAIbuffer) {
		tg->EAICore.EAIbuffer = MALLOC(char *, tg->EAICore.EAIbufsize * sizeof (char));
		if(eaiverbose) {
			printf("fwl_MIDI_handleBuffer() did not have a buffer, so create one at %p\n",tg->EAICore.EAIbuffer) ;
		}
	}
	if(eaiverbose) {
		printf("%s:%d fwl_MIDI_handleBuffer: Buffer at %p is %d chars,",__FILE__,__LINE__,fromFront,len);
		printf("Copy to buffer at %p\n", tg->EAICore.EAIbuffer);
	}

	if(len <= EAIREADSIZE) {
		tg->EAICore.EAIbuffer[len] = '\0';
		memcpy(tg->EAICore.EAIbuffer, fromFront, len);

		tg->EAICore.EAIbufpos = 0;
                tg->EAICore.EAIbufcount = 0;

		EAI_core_commands() ;

		th = &tg->EAIHelpers;
		return th->outBuffer ;
	} else {
		fwlio_RxTx_control(CHANNEL_MIDI,RxTx_STOP) ;
		return "";
	}
}

void EAI_core_commands () {
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
	//ppEAICore ps;
	struct tEAIHelpers *th;
	ttglobal tg = gglobal();
	p = (ppEAIEventsIn)tg->EAIEventsIn.prv;
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;
	th = &tg->EAIHelpers;
	//ps = tg->EAICore.prv;

	/* initialization */
	bufPtr = tg->EAICore.EAIbufpos;

	/* output buffer - start off with it this size */
	th->outBufferLen = EAIREADSIZE;
	th->outBuffer = MALLOC(char *, th->outBufferLen);
	th->outBuffer[0] = 0;

	if (EAI_BUFFER_CUR> 0) {
		if (eaiverbose) {
			printf ("EAI_core_commands: strlen %d str :%s:\n",(int)strlen((&EAI_BUFFER_CUR)), (&EAI_BUFFER_CUR));
		}

		/* step 1, get the command sequence number */
		if (sscanf ((&EAI_BUFFER_CUR),"%d",&count) != 1) {
			printf ("EAI_core_commands, expected a sequence number on command :%s:\n",(&EAI_BUFFER_CUR));
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
				dumpname = TEMPNAM(gglobal()->Mainloop.tmpFileLocation,"fwtmp");
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
				if (eaiverbose) { printf ("\n"); } /* need to fix the dangling printf before the case statement */
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
				if (eaiverbose) { printf ("\n"); } /* need to fix the dangling printf before the case statement */
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
				th->outBuffer[0] = 0;
				if (eaiverbose) {	
					printf ("after SENDEVENT, strlen %d\n",(int)strlen(&EAI_BUFFER_CUR));
				}
				break;
				}
			case MIDIINFO: {
/* if we do not have a string yet, we have to do this...
This is a problem. We cannot create the VRML until we have a whole stanza
which means we may have to block, because we cannot respond to the original request.

However, nowadays we do not read any sockets directly....
*/
				int topWaitLimit=16;
				int currentWaitCount=0;
				EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");

				while (EOT == NULL && topWaitLimit >= currentWaitCount) {
					if(fwlio_RxTx_control(CHANNEL_MIDI,RxTx_REFRESH) == 0) {
						/* Nothing to be done, maybe not even running */
						usleep(10000);
						currentWaitCount++;
					} else {
						if(fwlio_RxTx_waitfor(CHANNEL_MIDI,"\nEOT\n") != (char *)NULL) {
							char *tempEAIdata = fwlio_RxTx_getbuffer(CHANNEL_MIDI) ;
							if(tempEAIdata != (char *)NULL) {
								strcat(&EAI_BUFFER_CUR,tempEAIdata) ;
								/* tg->EAICore.EAIbuffer = ....*/
								free(tempEAIdata) ;
							}
						} else {
							usleep(10000);
							currentWaitCount++;
						}
					}
					EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");
				}

				if (topWaitLimit <= currentWaitCount) {
					/* Abandon Ship */
					sprintf (th->outBuffer,"RE\n%f\n%d\n-1",TickTime(),count);
				} else {
					*EOT = 0; /* take off the EOT marker*/
					/* MIDICODE ReWireRegisterMIDI(&EAI_BUFFER_CUR); */

					/* finish this, note the pointer maths */
					bufPtr = (int) (EOT+3-tg->EAICore.EAIbuffer);
					sprintf (th->outBuffer,"RE\n%f\n%d\n0",TickTime(),count);
				}
				break;
				}
			case MIDICONTROL: {
				/* sprintf (th->outBuffer,"RE\n%f\n%d\n%d",TickTime(),count, ReWireMIDIControl(&EAI_BUFFER_CUR)); */
				/* MIDICODE ReWireMIDIControl(&EAI_BUFFER_CUR); */
				break;
				}
			case CREATEVU:
			case CREATEVS: {
				/*format int seq# COMMAND vrml text     string EOT*/

				retGroup = createNewX3DNode(NODE_Group);
				if (command == CREATEVS) {
					int topWaitLimit=16;
					int currentWaitCount=0;

					if (eaiverbose) {	
						printf ("CREATEVS %s\n",&EAI_BUFFER_CUR);
					}	

					EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");
/* if we do not have a string yet, we have to do this...
This is a problem. We cannot create the VRML until we have a whole stanza
which means we may have to block, because we cannot respond to the original request.

However, nowadays we do not read any sockets directly....
*/

					while (EOT == NULL && topWaitLimit >= currentWaitCount) {
						if(fwlio_RxTx_control(CHANNEL_EAI,RxTx_REFRESH) == 0) {
							/* Nothing to be done, maybe not even running */
							usleep(10000);
							currentWaitCount++;
						} else {
							if(fwlio_RxTx_waitfor(CHANNEL_EAI,"\nEOT\n") != (char *)NULL) {
								char *tempEAIdata = fwlio_RxTx_getbuffer(CHANNEL_EAI) ;
								if(tempEAIdata != (char *)NULL) {
									strcat(&EAI_BUFFER_CUR,tempEAIdata) ;
									/* tg->EAICore.EAIbuffer = ....*/
									free(tempEAIdata) ;
								}
							} else {
								usleep(10000);
								currentWaitCount++;
							}
						}
						EOT = strstr(&EAI_BUFFER_CUR,"\nEOT\n");
					}
					if (topWaitLimit <= currentWaitCount) {
						/* Abandon Ship */
						sprintf (th->outBuffer,"RE\n%f\n%d\n-1",TickTime(),count);
					} else {

						*EOT = 0; /* take off the EOT marker*/

						ra = EAI_CreateVrml("String",(&EAI_BUFFER_CUR),retGroup);
						/* finish this, note the pointer maths */
						bufPtr = (int) (EOT+3-tg->EAICore.EAIbuffer);
					}
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
					printf ("SENDCHILD Parent: %u ParentField: %u %s Child at: %d\n",(unsigned int) ra, (unsigned int)rb, ctmp, rc);
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

				CRoutes_Register  (1,node, offset, X3D_NODE(tg->EAICore.EAIListenerData), 0, (int) tmp_c,(void *) 
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
				CRoutes_Register  (0,node, offset, X3D_NODE(tg->EAICore.EAIListenerData), 0, (int) tmp_c,(void *) 
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
				/*
				 * Lots of pretending  going on here....
				 *
				 * We prep (and send) the first portion of the reply
				 * but you will notice later on, we do not add an END_OF marker.
				 * So, then it is EAI_Anchor_Response() that sends the actual
				 * success/fail response with a END_OF marker.
				 *
				 * Therefore the client needs to be clever and glue the two-part
				 * reply back together again. This is easy in the socket code, but
				 * if you use functions, you need to be clever and wait for the
				 * async callback befor proceeding as if the request worked.
				 *
				 */

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

		/* skip to the next command */
		while (EAI_BUFFER_CUR >= ' ') bufPtr++;
		/* skip any new lines that may be there */
		while ((EAI_BUFFER_CUR == 10) || (EAI_BUFFER_CUR == 13)) bufPtr++;

		if (eaiverbose) {	
			printf ("end of command, remainder %d chars ",(int)strlen(&EAI_BUFFER_CUR));
#ifdef _MSC_VER
			printf ("and :%s: thread %lu\n",(&EAI_BUFFER_CUR),(unsigned long) pthread_self().p);
#else
			printf ("and :%s: thread %lu\n",(&EAI_BUFFER_CUR),(unsigned long) pthread_self());
#endif

		}
		if (command == SENDEVENT || (command == MIDICONTROL))  {
			/* events don't send a reply as such so your code has to check for zerolength strings*/
			th->outBuffer[0] = 0;
		} else {
			/* send the response, but only a portion in the case of LOADURL */
			if (command != LOADURL) outBufferCat("\nRE_EOT"); /* Please read and digest the LOADURL case above */
		}
	}
	tg->EAICore.EAIbufpos = bufPtr;
	return ;
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
	//ppEAICore ps;
	ttglobal tg = gglobal();
	p = (ppEAIEventsIn)tg->EAIEventsIn.prv;
	//ps = (ppEAICore)tg->EAICore.prv;
	if (p->waiting_for_anchor) {
		if (resp) strcpy (myline,"OK\nRE_EOT");
		else strcpy (myline,"FAIL\nRE_EOT");
		fwlio_RxTx_sendbuffer (__FILE__,__LINE__,CHANNEL_EAI,myline);
	}
	p->waiting_for_anchor = FALSE;
}

#endif //EXCLUDE_EAI
