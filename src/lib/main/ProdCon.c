/*
  $Id: ProdCon.c,v 1.108 2012/08/28 15:33:52 crc_canada Exp $

  Main functions II (how to define the purpose of this file?).
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
#include <system_threads.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>
#include <resources.h>

#include <io_files.h>
#include <io_http.h>

#include <resources.h>

#include <threads.h>

#include "../vrml_parser/Structs.h"
#include "headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CParse.h"
#include "../world_script/jsUtils.h"
#include "Snapshot.h"
#include "../scenegraph/Collision.h"

#if defined(INCLUDE_NON_WEB3D_FORMATS)
#include "../non_web3d_formats/ColladaParser.h"
#endif //INCLUDE_NON_WEB3D_FORMATS

#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"
#include "../input/InputFunctions.h"

#include "../plugin/pluginUtils.h"
#include "../plugin/PluginSocket.h"

#include "../ui/common.h"

#include "../opengl/Textures.h"
#include "../opengl/LoadTextures.h"

#include "ProdCon.h"

/* used by the paser to call back the lexer for EXTERNPROTO */
void embedEXTERNPROTO(struct VRMLLexer *me, char *myName, char *buffer, char *pound);

//true statics:
static char *EAI_Flag = "From the EAI bootcamp of life ";
char* PluginPath = "/private/tmp";
int PluginLength = 12;

//I think these are Aqua - I'll leave them static pending Aqua guru plugin review:
int _fw_browser_plugin = 0;
int _fw_pipe = 0;
uintptr_t _fw_instance = 0;

/* make up a new parser for parsing from createVrmlFromURL and createVrmlFromString */
//struct VRMLParser* savedParser;


/*
   ==============================================
   Explanations for this horrible modification :P
   ==============================================

   There is no reason to stop the main neither the display 
   while parser is parsing ;)... No reason I see with my little
   knowledge of the code...

   However, shared data access should be protected via mutex. I
   protect access to the download list (resource_list_to_parse).

   Root tree should also be protected when about to be modified.

*/

/*******************************/

///* thread synchronization issues */
//int _P_LOCK_VAR = 0;

#define SEND_TO_PARSER \
	if (p->_P_LOCK_VAR==0) { \
		p->_P_LOCK_VAR=1; \
	} \
	else ConsoleMessage ("SEND_TO_PARSER = flag wrong!\n");

#define PARSER_FINISHING \
	if (p->_P_LOCK_VAR==1) { \
		p->_P_LOCK_VAR=0; \
	} \
	else ConsoleMessage ("PARSER_FINISHING - flag wrong!\n");

#define UNLOCK \
	pthread_cond_signal(&gglobal()->threads.resource_list_condition); pthread_mutex_unlock(&gglobal()->threads.mutex_resource_list); 

#define WAIT_WHILE_PARSER_BUSY \
	pthread_mutex_lock(&gglobal()->threads.mutex_resource_list); \
     	while (p->_P_LOCK_VAR==1) { pthread_cond_wait(&gglobal()->threads.resource_list_condition, &gglobal()->threads.mutex_resource_list);} 


#define WAIT_WHILE_NO_DATA \
	pthread_mutex_lock(&gglobal()->threads.mutex_resource_list); \
     	while (p->_P_LOCK_VAR==0) { pthread_cond_wait(&gglobal()->threads.resource_list_condition, &gglobal()->threads.mutex_resource_list);} 








/*******************************/

//static s_list_t *resource_list_to_parse = NULL;

#define PARSE_STRING(input,len,where) parser_do_parse_string(input,len,where)

struct PSStruct {
	unsigned type;		/* what is this task? 			*/
	char *inp;		/* data for task (eg, vrml text)	*/
	void *ptr;		/* address (node) to put data		*/
	unsigned ofs;		/* offset in node for data		*/
	int zeroBind;		/* should we dispose Bindables?	 	*/
	int bind;		/* should we issue a bind? 		*/
	char *path;		/* path of parent URL			*/
	int *comp;		/* pointer to complete flag		*/

	char *fieldname;	/* pointer to a static field name	*/
	int jparamcount;	/* number of parameters for this one	*/
	struct Uni_String *sv;			/* the SV for javascript		*/
};

static bool parser_do_parse_string(const unsigned char *input, const int len, struct X3D_Group *nRn);

/* Bindables */
typedef struct pProdCon{
		struct Vector *fogNodes;
		struct Vector *backgroundNodes;
		struct Vector *navigationNodes;

		/* thread synchronization issues */
		int _P_LOCK_VAR;// = 0;
		s_list_t *resource_list_to_parse;// = NULL;
		/* psp is the data structure that holds parameters for the parsing thread */
		struct PSStruct psp;
		/* is the inputParse thread created? */
		int inputParseInitialized; //=FALSE;

		/* is the parsing thread active? this is read-only, used as a "flag" by other tasks */
		int inputThreadParsing; //=FALSE;
		int haveParsedCParsed;// = FALSE; 	/* used to tell when we need to call destroyCParserData  as destroyCParserData can segfault otherwise */

}* ppProdCon;
void *ProdCon_constructor(){
	void *v = malloc(sizeof(struct pProdCon));
	memset(v,0,sizeof(struct pProdCon));
	return v;
}
void ProdCon_init(struct tProdCon *t)
{
	//public
	t->viewpointNodes = newVector(struct X3D_Node *,8);
	t->currboundvpno=0;

	/* bind nodes in display loop, NOT in parsing threadthread */
	t->setViewpointBindInRender = NULL;
	t->setFogBindInRender = NULL;
	t->setBackgroundBindInRender = NULL;
	t->setNavigationBindInRender = NULL;
	t->savedParser = NULL; //struct VRMLParser* savedParser;

	//private
	t->prv = ProdCon_constructor();
	{
		ppProdCon p = (ppProdCon)t->prv;
		p->fogNodes = newVector(struct X3D_Node *, 2);
		p->backgroundNodes = newVector(struct X3D_Node *, 2);
		p->navigationNodes = newVector(struct X3D_Node *, 2);

		/* thread synchronization issues */
		p->_P_LOCK_VAR = 0;
		p->resource_list_to_parse = NULL;
		/* psp is the data structure that holds parameters for the parsing thread */
		//p->psp;
		/* is the inputParse thread created? */
		p->inputParseInitialized=FALSE;
		/* is the parsing thread active? this is read-only, used as a "flag" by other tasks */
		p->inputThreadParsing=FALSE;
		p->haveParsedCParsed = FALSE; 	/* used to tell when we need to call destroyCParserData 
				   as destroyCParserData can segfault otherwise */

	}
}
///* is the inputParse thread created? */
//static int inputParseInitialized=FALSE;
//
///* is the parsing thread active? this is read-only, used as a "flag" by other tasks */
//int inputThreadParsing=FALSE;

/* /\* Is the initial URL loaded ? Robert Sim *\/ */
/* int URLLoaded = FALSE; */
/* int isURLLoaded() { return (URLLoaded && !inputThreadParsing); } */

///* psp is the data structure that holds parameters for the parsing thread */
//struct PSStruct psp;

//static int haveParsedCParsed = FALSE; 	/* used to tell when we need to call destroyCParserData 
//				   as destroyCParserData can segfault otherwise */

/* is a parser running? this is a function, because if we need to mutex lock, we
   can do all locking in this file */
int fwl_isInputThreadInitialized() {
	ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;
	return p->inputParseInitialized;
}

/* statusbar uses this to tell user that we are still loading */
int fwl_isinputThreadParsing() {
	ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;
	return(p->inputThreadParsing);
}

/**
 *   parser_do_parse_string: actually calls the parser.
 */
static bool parser_do_parse_string(const unsigned char *input, const int len, struct X3D_Group *nRn)
{
	bool ret;
	ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;
	ret = FALSE;

	inputFileType = determineFileType(input,len);
	DEBUG_MSG("PARSE STRING, ft %d, fv %d.%d.%d\n",
		  inputFileType, inputFileVersion[0], inputFileVersion[1], inputFileVersion[2]);

	switch (inputFileType) {
	case IS_TYPE_XML_X3D:
		ret = X3DParse(nRn, (const char*)input);
		break;
	case IS_TYPE_VRML:
		ret = cParse(nRn,(int) offsetof (struct X3D_Group, children), (const char*)input);
		p->haveParsedCParsed = TRUE;
		break;
	case IS_TYPE_VRML1: {
		char *newData = convert1To2((const char*)input);
		ret = cParse (nRn,(int) offsetof (struct X3D_Group, children), newData);
		FREE_IF_NZ(newData);
		break;
    }
            
#if defined (INCLUDE_NON_WEB3D_FORMATS)
	case IS_TYPE_COLLADA:
		ConsoleMessage ("Collada not supported yet");
		ret = ColladaParse (nRn, (const char*)input);
		break;
	case IS_TYPE_SKETCHUP:
		ConsoleMessage ("Google Sketchup format not supported yet");
		break;
	case IS_TYPE_KML:
		ConsoleMessage ("KML-KMZ  format not supported yet");
		break;
#endif //INCLUDE_NON_WEB3D_FORMATS

#if defined (INCLUDE_STL_FILES)
        case IS_TYPE_ASCII_STL: {
            char *convertAsciiSTL(const unsigned char*);

            char *newData = convertAsciiSTL(input);
            //ConsoleMessage("IS_TYPE_ASCII_STL, now file is :%s:",newData);

            ret = cParse (nRn,(int) offsetof (struct X3D_Group, children), newData);
            FREE_IF_NZ(newData);
            break;
        }
        case IS_TYPE_BINARY_STL: {
            char *convertBinarySTL(const unsigned char*);
            char *newData = convertBinarySTL(input);
            ret = cParse (nRn,(int) offsetof (struct X3D_Group, children), newData);
            FREE_IF_NZ(newData);
            break;
        }
#endif //INCLUDE_STL_FILES

	default: {
		if (gglobal()->internalc.global_strictParsing) { ConsoleMessage ("unknown text as input"); } else {
			inputFileType = IS_TYPE_VRML;
			inputFileVersion[0] = 2; /* try VRML V2 */
			cParse (nRn,(int) offsetof (struct X3D_Group, children), (const char*)input);
			p->haveParsedCParsed = TRUE; }
	}
	}

    
	if (!ret) {
		ConsoleMessage ("Parser Unsuccessful");
	}
	return ret;
}

/* interface for telling the parser side to forget about everything...  */
void EAI_killBindables (void) {
	int complete;
	ttglobal tg = gglobal();
	ppProdCon p = (ppProdCon)tg->ProdCon.prv;

	WAIT_WHILE_PARSER_BUSY;

	complete=0;
	p->psp.comp = &complete;
	p->psp.type = ZEROBINDABLES;
	p->psp.ofs = 0;
	p->psp.ptr = NULL;
	p->psp.path = NULL;
	p->psp.zeroBind = FALSE;
	p->psp.bind = FALSE; /* should we issue a set_bind? */
	p->psp.inp = NULL;
	p->psp.fieldname = NULL;

	/* send data to a parser */
	SEND_TO_PARSER;

	UNLOCK;

	/* wait for data */
	WAIT_WHILE_PARSER_BUSY;

	/* grab data */
	UNLOCK;
}

/* interface for creating VRML for EAI */
int EAI_CreateVrml(const char *tp, const char *inputstring, struct X3D_Group *where)
{
	resource_item_t *res;
	char *newString;

	newString = NULL;

	if (strncmp(tp, "URL", 3) == 0) {

		res = resource_create_single(inputstring);
		res->where = where;
		res->offsetFromWhere = (int) offsetof (struct X3D_Group, children);
		/* printf ("EAI_CreateVrml, res->where is %u, root is %u parameter where %u\n",res->where, rootNode, where); */

	} else { // all other cases are inline code to parse... let the parser do the job ;P...

		const char *sendIn;

		if (strncmp(inputstring,"#VRML V2.0", 6) == 0) {
			sendIn = inputstring;
		} else {
			newString = MALLOC (char *, strlen(inputstring) + strlen ("#VRML V2.0 utf8\n") + 3);
			strcpy (newString,"#VRML V2.0 utf8\n");
			strcat (newString,inputstring);
			sendIn = newString;
			/* printf ("EAI_Create, had to append, now :%s:\n",newString); */
		}

		res = resource_create_from_string(sendIn);
		res->media_type=resm_vrml;
		res->parsed_request = EAI_Flag;
		res->where = where;
		res->offsetFromWhere = (int) offsetof (struct X3D_Group, children);
	}

	send_resource_to_parser(res,__FILE__,__LINE__);
	resource_wait(res);
	FREE_IF_NZ(newString);
	return (res->status == ress_parsed);
}

void send_resource_to_parser(resource_item_t *res,char *fi, int li)
{

	// ConsoleMessage ("send_resource_to_parser from %s:%d",fi,li);

	if (res->new_root) {
		//ConsoleMessage("send_resource_to_parser, new_root\n");
        	/* mark all rootNode children for Dispose */
		int i;
        	for (i=0; i<rootNode()->children.n; i++) {
                	markForDispose(rootNode()->children.p[i], TRUE);
        	}

		// force rootNode to have 0 children, compile_Group will make
		// the _sortedChildren field mimic the children field.
		rootNode()->children.n = 0; rootNode()->_change ++;

		//printf ("send_resource_to_parser, rootnode children count set to 0\n");

	}


	/* We are not in parser thread, most likely
	   in main or display thread, and we successfully
	   parsed a resource request.

	   We send it to parser.
	*/
	ppProdCon p = gglobal()->ProdCon.prv;

	/* Wait for display thread to be fully initialized */
	while (IS_DISPLAY_INITIALIZED == FALSE) {
		usleep(50);
	}

	/* wait for the parser thread to come up to speed */
	while (!p->inputParseInitialized) usleep(50);

	/* Lock access to the resource list */
	WAIT_WHILE_PARSER_BUSY;
 
	/* Add our resource item */
	p->resource_list_to_parse = ml_append(p->resource_list_to_parse, ml_new(res));

	/* signal that we have data on resource list */

	SEND_TO_PARSER;
	/* Unlock the resource list */
	UNLOCK;

	/* wait for the parser to finish */
	WAIT_WHILE_PARSER_BUSY;
	
	/* grab any data we want */
	UNLOCK;
}



void send_resource_to_parser_async(resource_item_t *res,char *fi, int li)
{
	/* We are not in parser thread, most likely
	   in main or display thread, and we successfully
	   parsed a resource request.

	   We send it to parser.
	*/
	ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;

	/* Wait for display thread to be fully initialized */
	while (IS_DISPLAY_INITIALIZED == FALSE) {
		usleep(50);
	}

	/* wait for the parser thread to come up to speed */
	while (!p->inputParseInitialized) usleep(50);

	/* Lock access to the resource list */
	WAIT_WHILE_PARSER_BUSY;
 
	/* Add our resource item */
	p->resource_list_to_parse = ml_append(p->resource_list_to_parse, ml_new(res));

	/* signal that we have data on resource list */

	SEND_TO_PARSER;
	/* Unlock the resource list */
	UNLOCK;

	/* wait for the parser to finish */
	//WAIT_WHILE_PARSER_BUSY;
	
	/* grab any data we want */
	//UNLOCK;
}

void dump_resource_waiting(resource_item_t* res)
{
#ifdef FW_DEBUG
	printf("%s\t%s\n",( res->complete ? "<finished>" : "<waiting>" ), res->request);
#endif
}



void dump_parser_wait_queue()
{
#ifdef FW_DEBUG
	ppProdCon p;
	struct tProdCon *t = &gglobal()->ProdCon;
	p = (ppProdCon)t->prv;
	printf("Parser wait queue:\n");
	ml_foreach(p->resource_list_to_parse, dump_resource_waiting((resource_item_t*)ml_elem(__l)));
	printf(".\n");
#endif
}

/**
 *   parser_process_res_VRML_X3D: this is the final parser (loader) stage, then call the real parser.
 */
static bool parser_process_res_VRML_X3D(resource_item_t *res)
{
	s_list_t *l;
	openned_file_t *of;
	struct X3D_Group *nRn;
	struct X3D_Group *insert_node;
	int i;
	int offsetInNode;
	int shouldBind;
    int parsedOk = FALSE; // results from parser
	/* we only bind to new nodes, if we are adding via Inlines, etc */
	int origFogNodes, origBackgroundNodes, origNavigationNodes, origViewpointNodes;
	ppProdCon p;
	struct tProdCon *t;
	ttglobal tg = gglobal();
	t = &tg->ProdCon;
	p = (ppProdCon)t->prv;


	/* printf("processing VRML/X3D resource: %s\n", res->request);  */
	
	shouldBind = FALSE;
	origFogNodes = vectorSize(p->fogNodes);
	origBackgroundNodes = vectorSize(p->backgroundNodes);
	origNavigationNodes = vectorSize(p->navigationNodes);
	origViewpointNodes = vectorSize(t->viewpointNodes);

	/* save the current URL so that any local-url gets are relative to this */
	pushInputResource(res);


	/* OK Boyz - here we go... if this if from the EAI, just parse it, as it will be a simple string */
	if (strcmp(res->parsed_request,EAI_Flag)==0) {

		/* EAI/SAI parsing */
		/* printf ("have the actual text here \n"); */
		/* create a container so that the parser has a place to put the nodes */
		nRn = (struct X3D_Group *) createNewX3DNode(NODE_Group);
	
		insert_node = X3D_GROUP(res->where); /* casting here for compiler */
		offsetInNode = res->offsetFromWhere;

		parsedOk = PARSE_STRING((const unsigned char *)res->request,(const int)strlen(res->request), nRn);
	} else {
		/* standard file parsing */
		l = (s_list_t *) res->openned_files;
		if (!l) {
			/* error */
			return FALSE;
		}
	
		of = ml_elem(l);
		if (!of) {
			/* error */
			return FALSE;
		}

	
		if (!of->fileData) {
			/* error */
			return FALSE;
		}

		/* 
		printf ("res %p root_res %p\n",res,gglobal()->resources.root_res); 
		ConsoleMessage ("pc - res %p root_res %p\n",res,gglobal()->resources.root_res); 
		*/

		/* bind ONLY in main - do not bind for Inlines, etc */
		if (res == gglobal()->resources.root_res) {
			kill_bindables();
			shouldBind = TRUE;
			//ConsoleMessage ("pc - shouldBind");
		} else {
			if (!tg->resources.root_res->complete) {
				/* Push the parser state : re-entrance here */
				/* "save" the old classic parser state, so that names do not cross-pollute */
				t->savedParser = (void *)tg->CParse.globalParser;
				tg->CParse.globalParser = NULL;
			}
		}

		/* create a container so that the parser has a place to put the nodes */
		nRn = (struct X3D_Group *) createNewX3DNode(NODE_Group);
	
		/* ACTUALLY CALLS THE PARSER */
		parsedOk = PARSE_STRING(of->fileData, of->fileDataSize, nRn);
	
		if ((res != tg->resources.root_res) && ((!tg->resources.root_res) ||(!tg->resources.root_res->complete))) {
			tg->CParse.globalParser = t->savedParser;
		}
	
		if (shouldBind) {
			if (vectorSize(p->fogNodes) > 0) {
				for (i=origFogNodes; i < vectorSize(p->fogNodes); ++i) 
					send_bind_to(vector_get(struct X3D_Node*,p->fogNodes,i), 0); 
					/* Initialize binding info */
				t->setFogBindInRender = vector_get(struct X3D_Node*, p->fogNodes,0);
			}
			if (vectorSize(p->backgroundNodes) > 0) {
				for (i=origBackgroundNodes; i < vectorSize(p->backgroundNodes); ++i) 
					send_bind_to(vector_get(struct X3D_Node*,p->backgroundNodes,i), 0); 
					/* Initialize binding info */
				t->setBackgroundBindInRender = vector_get(struct X3D_Node*, p->backgroundNodes,0);
			}
			if (vectorSize(p->navigationNodes) > 0) {
				for (i=origNavigationNodes; i < vectorSize(p->navigationNodes); ++i) 
					send_bind_to(vector_get(struct X3D_Node*,p->navigationNodes,i), 0); 
					/* Initialize binding info */
				t->setNavigationBindInRender = vector_get(struct X3D_Node*, p->navigationNodes,0);
			}
			if (vectorSize(t->viewpointNodes) > 0) {
				for (i=origViewpointNodes; i < vectorSize(t->viewpointNodes); ++i) 
					send_bind_to(vector_get(struct X3D_Node*,t->viewpointNodes,i), 0); 
					/* Initialize binding info */
				t->setViewpointBindInRender = vector_get(struct X3D_Node*, t->viewpointNodes,0);
			}
		}
	
		/* we either put things at the rootNode (ie, a new world) or we put them as a children to another node */
		if (res->where == NULL) {
			ASSERT(rootNode());
			insert_node = rootNode();
			offsetInNode = (int) offsetof(struct X3D_Group, children);
		} else {
			insert_node = X3D_GROUP(res->where); /* casting here for compiler */
			offsetInNode = res->offsetFromWhere;
		}
	}
	
	/* printf ("parser_process_res_VRML_X3D, res->where %u, insert_node %u, rootNode %u\n",res->where, insert_node, rootNode); */

	/* now that we have the VRML/X3D file, load it into the scene. */
	/* add the new nodes to wherever the caller wanted */

	/* take the nodes from the nRn node, and put them into the place where we have decided to put them */
	AddRemoveChildren(X3D_NODE(insert_node),
			  offsetPointer_deref(void*, insert_node, offsetInNode), 
			  (struct X3D_Node * *)nRn->children.p,
			  nRn->children.n, 1, __FILE__,__LINE__);
	
	/* and, remove them from this nRn node, so that they are not multi-parented */
	AddRemoveChildren(X3D_NODE(nRn),
			  (struct Multi_Node *)((char *)nRn + offsetof (struct X3D_Group, children)),
			  (struct X3D_Node* *)nRn->children.p,nRn->children.n,2,__FILE__,__LINE__);	

	res->complete = TRUE;
	

	/* remove this resource from the stack */
	popInputResource();

	return TRUE;
}

/**
 *   parser_process_res_SHADER: this is the final parser (loader) stage, then call the real parser.
 */
static bool parser_process_res_SHADER(resource_item_t *res)
{
	s_list_t *l;
	openned_file_t *of;
	struct Shader_Script* ss;
	const char *buffer;

	buffer = NULL;

	switch (res->type) {
	case rest_invalid:
		return FALSE;
		break;

	case rest_string:
		buffer = res->request;
		break;
	case rest_url:
	case rest_file:
	case rest_multi:
		l = (s_list_t *) res->openned_files;
		if (!l) {
			/* error */
			return FALSE;
		}
		
		of = ml_elem(l);
		if (!of) {
			/* error */
			return FALSE;
		}

		/* FIXME: finish this */
		break;
	}

	ss = (struct Shader_Script *) res->where;
	
	return script_initCode(ss, buffer);
}

/**
 *   parser_process_res: for each resource state, advance the process of loading.
 */
static bool parser_process_res(s_list_t *item)
{
	bool remove_it = FALSE;
	resource_item_t *res;
	ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;
    bool retval = TRUE;

	if (!item || !item->elem)
		return retval;

	res = ml_elem(item);

	//printf("\nprocessing resource: type %s, status %s\n", resourceTypeToString(res->type), resourceStatusToString(res->status)); 
	switch (res->status) {

	case ress_invalid:
	case ress_none:
            retval = FALSE;
		resource_identify(res->parent, res);
		if (res->type == rest_invalid) {
			remove_it = TRUE;
		}
		break;

	case ress_starts_good:
		resource_fetch(res);
		break;

	case ress_downloaded:
		/* Here we may want to delegate loading into another thread ... */
		if (!resource_load(res)) {
			ERROR_MSG("failure when trying to load resource: %s\n", res->request);
			remove_it = TRUE;
			retval = FALSE;
		}
		break;

	case ress_failed:
            retval = FALSE;
		remove_it = TRUE;
		break;

	case ress_loaded:
		// printf("processing resource, media_type %s\n",resourceMediaTypeToString(res->media_type)); 
		switch (res->media_type) {
		case resm_unknown:
			ConsoleMessage ("deciphering file: 404 file not found or unknown file type encountered.");
			remove_it = TRUE;
			res->complete=TRUE; /* not going to do anything else with this one */
			res->status = ress_not_loaded;
			break;
		case resm_vrml:
		case resm_x3d:
			if (parser_process_res_VRML_X3D(res)) {
				DEBUG_MSG("parser successfull: %s\n", res->request);
				res->status = ress_parsed;
                
			} else {
				ERROR_MSG("parser failed for resource: %s\n", res->request);
                retval = FALSE;
			}
			break;
		case resm_pshader:
		case resm_fshader:
			if (parser_process_res_SHADER(res)) {
				DEBUG_MSG("parser successfull: %s\n", res->request);
				res->status = ress_parsed;
			} else {
                retval = FALSE;
				ERROR_MSG("parser failed for resource: %s\n", res->request);
			}
			break;
		case resm_image:
		case resm_movie:
			/* Texture file has been loaded into memory
			   the node could be updated ... i.e. texture created */
			res->complete = TRUE; /* small hack */
			break;
		}
		/* Parse only once ! */
		remove_it = TRUE;
		break;

	case ress_not_loaded:
		remove_it = TRUE;
            retval = FALSE;
		break;

	case ress_parsed:
		remove_it = TRUE;
		break;

	case ress_not_parsed:
            retval = FALSE;
		remove_it = TRUE;
		break;		
	}

	if (remove_it) {
		/* Remove the parsed resource from the list */
		p->resource_list_to_parse = ml_delete_self(p->resource_list_to_parse, item);

		/* What next ? */
//		dump_parser_wait_queue();
	}

	dump_parser_wait_queue();
    
	// printf ("end of process resource\n");

    return retval;
}

#if !defined(HAVE_PTHREAD_CANCEL)
void Parser_thread_exit_handler(int sig)
{
    ConsoleMessage("Parser_thread_exit_handler: parserThread exiting");
    pthread_exit(0);
}
#endif //HAVE_PTHREAD_CANCEL


/**
 *   _inputParseThread: parser (loader) thread.
 */

void _inputParseThread(void)
{
	ENTER_THREAD("input parser");

        #if !defined (HAVE_PTHREAD_CANCEL)
        struct sigaction actions;
        int rc;
        memset(&actions, 0, sizeof(actions));
        sigemptyset(&actions.sa_mask);
        actions.sa_flags = 0;
        actions.sa_handler = Parser_thread_exit_handler;
        rc = sigaction(SIGUSR2,&actions,NULL);
	// ConsoleMessage ("for parserThread, have defined exit handler");
        #endif //HAVE_PTHREAD_CANCEL

	{
		ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;
        bool result;

		p->inputParseInitialized = TRUE;

		viewer_default();

		/* now, loop here forever, waiting for instructions and obeying them */

		for (;;) {
			WAIT_WHILE_NO_DATA;

            result = TRUE;
			p->inputThreadParsing = TRUE;

			/* go through the resource list until it is empty */
			while (p->resource_list_to_parse != NULL) {
				ml_foreach(p->resource_list_to_parse, result=parser_process_res(__l));
                //printf ("ml_foreach, result %d, TRUE %d, false %d\n",result,TRUE,FALSE);
			}
			p->inputThreadParsing = FALSE;

#if defined (IPHONE) || defined (_ANDROID)
            
            if (result) setMenuStatus ("ok"); else setMenuStatus("not ok");
#endif

			/* Unlock the resource list */
			PARSER_FINISHING;

			UNLOCK;
		}
	}
}

static void unbind_node(struct X3D_Node* node) {
	switch (node->_nodeType) {
		case NODE_Viewpoint:
			X3D_VIEWPOINT(node)->isBound = 0;
			break;
		case NODE_OrthoViewpoint:
			X3D_ORTHOVIEWPOINT(node)->isBound = 0;
			break;
		case NODE_GeoViewpoint:
			X3D_GEOVIEWPOINT(node)->isBound = 0;
			break;
		case NODE_Background:
			X3D_BACKGROUND(node)->isBound = 0;
			break;
		case NODE_TextureBackground:
			X3D_TEXTUREBACKGROUND(node)->isBound = 0;
			break;
		case NODE_NavigationInfo:
			X3D_NAVIGATIONINFO(node)->isBound = 0;
			break;
		case NODE_Fog:
			X3D_FOG(node)->isBound = 0;
			break;
		default: {
			/* do nothing with this node */
			return;
		}                                                
	}
}
/* for ReplaceWorld (or, just, on start up) forget about previous bindables */
#define KILL_BINDABLE(zzz) \
	{ int i; for (i=0; i<vectorSize(zzz); i++) { \
		struct X3D_Node* me = vector_get(struct X3D_Node*,zzz,i); \
		unbind_node(me); \
	} \
	deleteVector(struct X3D_Node *,zzz); \
	zzz = newVector(struct X3D_Node *,8); \
	/*causes segfault, do not do this zzz = NULL;*/ \
	}


void kill_bindables (void) {
	ppProdCon p;
	struct tProdCon *t = &gglobal()->ProdCon;
	p = (ppProdCon)t->prv;


	KILL_BINDABLE(t->viewpointNodes);
	KILL_BINDABLE(p->backgroundNodes);
	KILL_BINDABLE(p->navigationNodes);
	KILL_BINDABLE(p->fogNodes);
	


	/* XXX MEMORY LEAK HERE
	FREE_IF_NZ(p->fognodes);
	FREE_IF_NZ(p->backgroundnodes);
	FREE_IF_NZ(p->navnodes);
	FREE_IF_NZ(t->viewpointnodes);
	*/
}


void registerBindable (struct X3D_Node *node) {
	ppProdCon p;
	struct tProdCon *t = &gglobal()->ProdCon;
	p = (ppProdCon)t->prv;

	
	switch (node->_nodeType) {
		case NODE_Viewpoint:
			X3D_VIEWPOINT(node)->set_bind = 100;
			X3D_VIEWPOINT(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,t->viewpointNodes, node);
			break;
		case NODE_OrthoViewpoint:
			X3D_ORTHOVIEWPOINT(node)->set_bind = 100;
			X3D_ORTHOVIEWPOINT(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,t->viewpointNodes, node);
			break;
		case NODE_GeoViewpoint:
			X3D_GEOVIEWPOINT(node)->set_bind = 100;
			X3D_GEOVIEWPOINT(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,t->viewpointNodes, node);
			break;
		case NODE_Background:
			X3D_BACKGROUND(node)->set_bind = 100;
			X3D_BACKGROUND(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,p->backgroundNodes, node);
			break;
		case NODE_TextureBackground:
			X3D_TEXTUREBACKGROUND(node)->set_bind = 100;
			X3D_TEXTUREBACKGROUND(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,p->backgroundNodes, node);
			break;
		case NODE_NavigationInfo:
			X3D_NAVIGATIONINFO(node)->set_bind = 100;
			X3D_NAVIGATIONINFO(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,p->navigationNodes, node);
			break;
		case NODE_Fog:
			X3D_FOG(node)->set_bind = 100;
			X3D_FOG(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,p->fogNodes, node);
			break;
		default: {
			/* do nothing with this node */
			/* printf ("got a registerBind on a node of type %s - ignoring\n",
					stringNodeType(node->_nodeType));
			*/
			return;
		}                                                

	}
}

