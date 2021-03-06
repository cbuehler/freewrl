
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
#include <stdio.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../world_script/fieldSet.h"
#include "../input/EAIHeaders.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CProto.h"


typedef struct pconvert1To2{
	int estimatedBodyLen;// = -1;
	struct Vector *deconstructedProtoBody;// = NULL;

	int protoElementCount;// = ID_UNDEFINED;
	char tempname[1000];
	FILE *fp;
	int written;// = 0;
}* ppconvert1To2;

void *convert1To2_constructor()
{
	void *v = malloc(sizeof(struct pconvert1To2));
	memset(v,0,sizeof(struct pconvert1To2));
	return v;
}
void convert1To2_init(struct tconvert1To2* t)
{
	//public
	//private
	ppconvert1To2 p;
	t->prv = convert1To2_constructor();
	p = (ppconvert1To2)t->prv;
	p->estimatedBodyLen = -1;
	p->deconstructedProtoBody = NULL;

	p->protoElementCount = ID_UNDEFINED;
	p->written = 0;
}


#define DEF_FINDFIELD(arr) \
 int findFieldIn##arr(const char* field) \
 { \
  return findFieldInARR(field, arr, arr##_COUNT); \
 }
DEF_FINDFIELD(VRML1_)
DEF_FINDFIELD(VRML1Modifier)

static struct ProtoElementPointer* newProtoElementPointer(void) {
        struct ProtoElementPointer *ret=MALLOC(struct ProtoElementPointer *,sizeof(struct ProtoElementPointer));
        ASSERT (ret);

        ret->stringToken = NULL;
        ret->isNODE = ID_UNDEFINED;
        ret->isKEYWORD = ID_UNDEFINED;
        ret->terminalSymbol = ID_UNDEFINED;
        ret->fabricatedDef = ID_UNDEFINED; /* this is unused here */
        return ret;
}

/* special marker for redefine the terminal symbol from "{" to "{children["... */
#define START_CHILDREN 300
#define END_CHILDREN 400

/* go through the remainder of the fields, and, if we have a "{ "}" pair, change it to a "{children[...]}" pair */
static void possiblyChangetoChildren (int startIndex) {
	indexT yy;
	indexT bracketCount;
       	struct ProtoElementPointer* tempEle;
	ppconvert1To2 p = gglobal()->convert1To2.prv;
	/* did we run off the end? */
	if (startIndex >= p->protoElementCount) return;

	/* this one had better be a "{" */
	yy = startIndex;
	tempEle = vector_get(struct ProtoElementPointer*, p->deconstructedProtoBody, yy);
	if (tempEle->terminalSymbol != '{') {
		printf ("did not find an open brace where expected\n");
		return;
	}
	tempEle->terminalSymbol = START_CHILDREN;
	bracketCount = 1;
	while (bracketCount >= 1) {
		tempEle = vector_get(struct ProtoElementPointer*, p->deconstructedProtoBody, yy);
		if (tempEle->terminalSymbol == '{') bracketCount ++;
		if (tempEle->terminalSymbol == '}') bracketCount --;
		yy++;
		/* did we go beyond the last element? */
		if (yy>p->protoElementCount) {
			printf ("did not find matching bracket for child node\n");
			return;
		}	
	}
	if (tempEle->terminalSymbol != '}') {
		printf ("did not find a close brace where expected\n");
		return;
	}
	tempEle->terminalSymbol = END_CHILDREN;
}

void tokenizeVRML1_(char *pb) {
	struct VRMLLexer *lex;
	vrmlInt32T tmp32;
	vrmlFloatT tmpfloat;
	vrmlStringT tmpstring;
	struct ProtoElementPointer* ele;
	int toPush;
	indexT ct = 0;
	ppconvert1To2 p = gglobal()->convert1To2.prv;

	/* remove spaces at start of string, to help to see if string is empty */
	while ((*pb != '\0') && (*pb <= ' ')) pb++;

	/* record this body length to help us with MALLOCing when expanding PROTO */
	p->estimatedBodyLen = (int) strlen(pb) * 2;

	lex = newLexer();
	lexer_fromString(lex,pb);

	/* make up deconstructedProtoBody here */
 	p->deconstructedProtoBody=newVector(struct ProtoElementPointer*, 128);
	ASSERT(p->deconstructedProtoBody);



	while (lex->isEof == FALSE) 
	{
		ele = newProtoElementPointer();
		toPush = TRUE; /* only put this new element on Vector if it is successful */

		if (lexer_setCurID(lex)) 
		{
			char tmpname[1000];
			strcpy (tmpname,"VRML1_");
			strcat (tmpname,lex->curID);
			if ((ele->isKEYWORD = (indexT) findFieldInVRML1_(tmpname)) == ID_UNDEFINED)  
			{
				indexT i;
					
				/* is this one of the VRML1 keywords that must be quoted? */
				i = findFieldInVRML1Modifier(lex->curID);
				if (i != ID_UNDEFINED) {
					ele->stringToken = MALLOC(char *, strlen(lex->curID) + 4);
					strcpy(ele->stringToken,"\"");
					strcat(ele->stringToken,lex->curID);
					strcat(ele->stringToken,"\"");
				} else {
					ele->stringToken = lex->curID;
					lex->curID = NULL;
				}
			} 
			FREE_IF_NZ(lex->curID);
			/* period not reserved in VRML1, and numbers can start with a period....
				} else if (lexer_point(lex)) { ele->terminalSymbol = (indexT) '.'; */

		} else if (lexer_openCurly(lex)) { ele->terminalSymbol = (indexT) '{';
		} else if (lexer_closeCurly(lex)) { ele->terminalSymbol = (indexT) '}';
		} else if (lexer_openSquare(lex)) { ele->terminalSymbol = (indexT) '[';
		} else if (lexer_closeSquare(lex)) { ele->terminalSymbol = (indexT) ']';
		} else if (lexer_colon(lex)) { ele->terminalSymbol = (indexT) ':';

		} else if (lexer_string(lex,&tmpstring)) { 
			/* must put the double quotes back on */
			ele->stringToken = MALLOC (char *, tmpstring->len + 3);
			sprintf (ele->stringToken, "\"%s\"",tmpstring->strptr);
		} else {
			/* printf ("probably a number, scan along until it is done. :%s:\n",lex->nextIn); */
			if ((*lex->nextIn == '.') ||(*lex->nextIn == '-') || ((*lex->nextIn >= '0') && (*lex->nextIn <= '9'))) 
			{
				uintptr_t ip; uintptr_t fp; char *cur;
				int ignore;

				/* see which of float, int32 gobbles up more of the string */
				cur = (char *) lex->nextIn;

				ignore = lexer_float(lex,&tmpfloat);
				fp = (uintptr_t) lex->nextIn;
				/* put the next in pointer back to the beginning of the number */
				lex->nextIn = cur;


				ignore = lexer_int32(lex,&tmp32);
				ip = (uintptr_t) lex->nextIn;

				/* put the next in pointer back to the beginning of the number */
				lex->nextIn = cur;

				ele->stringToken = MALLOC (char *, 12);
				ASSERT (ele->stringToken);

				/* printf ("so we read in from :%s:\n",lex->nextIn); */

				/* now, really scan depending on the type - which one got us further? */
				/* note that if they are the same, we choose the INT because of expansion
				   problems if we write an int as a float... (eg, coordinates in an IFS) */
				if (ip >= fp) {
					/* printf ("this is an int\n"); */
					ignore = lexer_int32(lex,&tmp32);
					sprintf (ele->stringToken,"%d",tmp32);
				} else {
					/* printf ("this is a float\n"); */
					ignore = lexer_float(lex,&tmpfloat); 
					sprintf (ele->stringToken,"%f",tmpfloat);
				}

			
			} else {
				if (*lex->nextIn != '\0') ConsoleMessage ("lexer_setCurID failed on char :%d:\n",*lex->nextIn);
				lex->nextIn++;
				toPush = FALSE;
			}
		}


		/* printf ("newprotoele %d, NODE %d KW %d ts %d st %s\n",ct, ele->isNODE, ele->isKEYWORD, ele->terminalSymbol, ele->stringToken); */
		ct ++;

		/* push this element on the vector for the PROTO */
		if (toPush) vector_pushBack(struct ProtoElementPointer*, p->deconstructedProtoBody, ele);
	}
	deleteLexer(lex);



	/* check the deconstructedProtoBody */
	/* if the user types in "DEF Material Material {}" the first "Material" is NOT a node, but a stringToken... */

	{
        indexT i;
        struct ProtoElementPointer* ele;

        /* go through each part of this deconstructedProtoBody, and see what needs doing... */
        p->protoElementCount = vectorSize(p->deconstructedProtoBody);
        i = 0;
	
		p->written = fprintf (p->fp,"#VRML V2.0 utf8\n");
		p->written += fprintf(p->fp,"VRML1_Separator 	#kw\n {VRML1children[	#child terminalSymbol\n"); //dug9 scene wrapper idea
        while (i < p->protoElementCount) 
		{
            /* get the current element */
            ele = vector_get(struct ProtoElementPointer*, p->deconstructedProtoBody, i);
	
            if (ele->isNODE != -1) 
                    p->written += fprintf (p->fp," VRML1_%s 		#NODE\n",stringNodeType(ele->isNODE));

			/* this is a keyword; lets see if this is a children type node */
            if (ele->isKEYWORD != -1) 
			{
                p->written += fprintf (p->fp," %s 	#kw\n",stringVRML1_Type(ele->isKEYWORD));

				/* find the brackets, as these might need to be changed into a colon */
				if (ele->isKEYWORD == VRML1_VRML1_Separator) 
				{
					possiblyChangetoChildren (i+1);
				}
			}

			if (ele->stringToken != NULL) 
					p->written += fprintf (p->fp," %s 		#string\n",ele->stringToken);

			if (ele->terminalSymbol != -1)  
			{
				if (ele->terminalSymbol == START_CHILDREN) {
                    			p->written += fprintf (p->fp," {VRML1children[	#child terminalSymbol\n");
				} else if (ele->terminalSymbol == END_CHILDREN) {
                    			p->written += fprintf (p->fp," ]}	#child terminalSymbol\n");
				} else {
                    			p->written += fprintf (p->fp," %c 		#terminalSymbol\n",(char) ele->terminalSymbol);
				}
			}
			i++;
		}
		p->written += fprintf(p->fp,"]}	#child terminalSymbol\n"); //dug9 scene wrapper idea
	}
}

char *convert1To2 (const char *inp)
{
	ttglobal tg = gglobal();

	char *retval = NULL;
	char *dinp, *tptr;
	size_t readSizeThrowAway;
	ppconvert1To2 p = tg->convert1To2.prv;

	/* sanitize input but copy data before altering it */
	dinp = tptr = strdup(inp);
	while (*tptr != '\0') {
		if ((*tptr < 0) || (*tptr > (char) 0x7d)) {
			printf ("found a char of %x\n",*tptr);
			*tptr = ' ';
		}
	tptr ++;
	}
	retval = NULL;

	/* FIXME: make use of new API */

	

	retval = TEMPNAM(tg->Mainloop.tmpFileLocation,"freewrl_tmp");
		

	sprintf (p->tempname, "%s",retval);
	free(retval); retval = NULL;
	p->fp = fopen (p->tempname,"w");
	if (p->fp != NULL) {
		
		tokenizeVRML1_(dinp);
		fclose(p->fp);

		FREE(dinp);
		
		p->fp = fopen(p->tempname,"r");
		retval = MALLOC(char *, p->written+10);
		readSizeThrowAway = fread(retval,p->written,1,p->fp);
		retval[p->written] = '\0';
		/* printf ("and have read back in :%s:\n",retval); */

		fclose(p->fp);
		return retval;
	}
	return STRDUP("Shape{geometry Box {}}");

}
