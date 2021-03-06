/*
  $Id: Textures.c,v 1.132 2012/08/29 20:26:09 crc_canada Exp $

  FreeWRL support library.
  Texture handling code.

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

#include <threads.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"

#include "../scenegraph/readpng.h"
#include "../input/InputFunctions.h"
#include "Textures.h"
#include <resources.h>
#include "../opengl/Material.h"
#include "../opengl/OpenGL_Utils.h"
#include "../world_script/fieldSet.h"
#include "../scenegraph/Component_Shape.h"
#include "../scenegraph/Component_CubeMapTexturing.h"
#include "../scenegraph/RenderFuncs.h"
#include "LoadTextures.h"

#ifdef AQUA
#ifndef IPHONE
# include <Carbon/Carbon.h>
# include <QuickTime/QuickTime.h>
#endif
#else
# if HAVE_JPEGLIB_H
#undef HAVE_STDLIB_H
#undef FAR
#  include <jpeglib.h>
#  include <setjmp.h>
# endif
#endif


static void new_bind_image(struct X3D_Node *node, struct multiTexParams *param);
textureTableIndexStruct_s *getTableIndex(int i);

typedef struct pTextures{
	struct Vector *activeTextureTable;
	textureTableIndexStruct_s* loadThisTexture;

	/* current index into loadparams that texture thread is working on */
	int currentlyWorkingOn;// = -1;
	int textureInProcess;// = -1;
}* ppTextures;


void *Textures_constructor(){
	void *v = malloc(sizeof(struct pTextures));
	memset(v,0,sizeof(struct pTextures));
	return v;
}
void Textures_init(struct tTextures *t){
	//public

	//private
	t->prv = Textures_constructor();
	{
		ppTextures p = (ppTextures)t->prv;
		p->activeTextureTable = NULL;

		/* current index into loadparams that texture thread is working on */
		p->currentlyWorkingOn = -1;

		p->textureInProcess = -1;
	}
}

#if defined(AQUA) /* for AQUA OS X sharing of OpenGL Contexts */

#elif defined(_MSC_VER)

#else
#if !defined(_ANDROID) && !defined(GLES2)
GLXContext textureContext = NULL;
#endif

#endif

/* function Prototypes */
int findTextureFile(textureTableIndexStruct_s *entry);
void _textureThread(void);

static void move_texture_to_opengl(textureTableIndexStruct_s*);
struct Uni_String *newASCIIString(char *str);

int readpng_init(FILE *infile, ulg *pWidth, ulg *pHeight);
void readpng_cleanup(int free_image_data);


static void myTexImage2D (int generateMipMaps, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLubyte *pixels);

/* gluScaleImage replacement */

/* we manipulate things here as 4 byte colours (RGBA, or BGRA) so we try and ensure
that we have 32 bit numbers, thus this definition. ES-2.0 is 32 bits, but in case
someone compiles on a 64 bit system, with Material shader definitions in OpenGL-3.x,
then hopefully this will work for them, too */

#ifndef uint32
# define uint32 uint32_t
#endif



static void myScaleImage(int srcX,int srcY,int destX,int destY,unsigned char *src, unsigned char *dest) {
	float YscaleFactor;
	float XscaleFactor;
	int wye, yex;
	uint32 *src32 = (uint32 *)src;
	uint32 *dest32 = (uint32 *)dest;

	if ((srcY<=0) || (destY<=0) || (srcX<=0) || (destX<=0)) return;
	if (src == NULL) return;
	if (dest == NULL) return;

	if ((srcY==destY) && (srcX==destX)) {
		/* printf ("simple copy\n"); */
		memcpy (dest,src,srcY*srcX*4); /* assuming FreeWRL-standard RGBA or BGRA textures */
	}

	/* do x direction first */
	YscaleFactor = ((float)srcY) / ((float)destY);
	XscaleFactor = ((float)srcX) / ((float)destX);

	for (wye=0; wye<destY; wye++) {
		for (yex=0; yex<destX; yex++) {
			float fx, fy;
			int row, column;
			int oldIndex;

			fx = YscaleFactor * ((float) wye);
			fy = XscaleFactor * ((float) yex);
			row = (int)(fx);
			column = (int)(fy);
			oldIndex = row * srcX + column; /* so many rows, each row has srcX columns */
			dest32[wye*destX+yex] = src32[oldIndex];
		}
	}
}


static void GenMipMap2D( GLubyte *src, GLubyte **dst, int srcWidth, int srcHeight, int *dstWidth, int *dstHeight )
{
   int x,
       y;
   int texelSize = 4;

   *dstWidth = srcWidth / 2;
   if ( *dstWidth <= 0 )
      *dstWidth = 1;

   *dstHeight = srcHeight / 2;
   if ( *dstHeight <= 0 )
      *dstHeight = 1;

   *dst = malloc ( sizeof(GLubyte) * texelSize * (*dstWidth) * (*dstHeight) );
   if ( *dst == NULL )
      return;

   for ( y = 0; y < *dstHeight; y++ )
   {
      for( x = 0; x < *dstWidth; x++ )
      {
         int srcIndex[4];
         float r = 0.0f,
               g = 0.0f,
               b = 0.0f,
	       a = 0.0f;

         int sample;

        // Compute the offsets for 2x2 grid of pixels in previous
         // image to perform box filter
         srcIndex[0] = 
            (((y * 2) * srcWidth) + (x * 2)) * texelSize;
         srcIndex[1] = 
            (((y * 2) * srcWidth) + (x * 2 + 1)) * texelSize; 
         srcIndex[2] = 
            ((((y * 2) + 1) * srcWidth) + (x * 2)) * texelSize;
         srcIndex[3] = 
            ((((y * 2) + 1) * srcWidth) + (x * 2 + 1)) * texelSize;

         // Sum all pixels
         for ( sample = 0; sample < 4; sample++ )
         {
            r += src[srcIndex[sample]];
            g += src[srcIndex[sample] + 1];
            b += src[srcIndex[sample] + 2];
            a += src[srcIndex[sample] + 3];
         }

         // Average results
         r /= 4.0f;
         g /= 4.0f;
         b /= 4.0f;
         a /= 4.0f;

         // Store resulting pixels
         (*dst)[ ( y * (*dstWidth) + x ) * texelSize ] = (GLubyte)( r );
         (*dst)[ ( y * (*dstWidth) + x ) * texelSize + 1] = (GLubyte)( g );
         (*dst)[ ( y * (*dstWidth) + x ) * texelSize + 2] = (GLubyte)( b );
         (*dst)[ ( y * (*dstWidth) + x ) * texelSize + 3] = (GLubyte)( a );
      }
   }
}

/* create the MIPMAPS ourselves, as the OpenGL ES 2.0 can not do it */
static void myTexImage2D (int generateMipMaps, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLubyte *pixels) {
	GLubyte *prevImage = NULL;
	GLubyte *newImage = NULL;
	
	/* first, base image */
	FW_GL_TEXIMAGE2D(target,level,internalformat,width,height,border,format,type,pixels);

	if (!generateMipMaps) return;
	if ((width <=1) && (height <=1)) return;


	/* go and create a bunch of mipmaps */

	prevImage = MALLOC(GLubyte *, width * height * 4);
	memcpy (prevImage, pixels, width * height * 4);
	
	/* from the OpenGL-ES 2.0 book, page 189 */
	level = 1;

	while ((width > 1) && (height > 1)) {
		GLint newWidth, newHeight;
		
		GenMipMap2D(prevImage,&newImage, width, height, &newWidth, &newHeight);

		FW_GL_TEXIMAGE2D(target,level,internalformat,newWidth,newHeight,0,format,GL_UNSIGNED_BYTE,newImage);

		FREE_IF_NZ(prevImage);
		prevImage = newImage;
		level++;
		width = newWidth;
		height = newHeight;				
	}

	FREE_IF_NZ(newImage);
}


/**
 *   texst: texture status string.
 */
const char *texst(int num)
{
	if (num == TEX_NOTLOADED) return "TEX_NOTLOADED";
	if (num == TEX_LOADING) return "TEX_LOADING";
	if (num == TEX_NEEDSBINDING)return "TEX_NEEDSBINDING";
	if (num == TEX_LOADED)return "TEX_LOADED";
	if (num == TEX_UNSQUASHED)return "UNSQUASHED";
	return "unknown";
}


/* does a texture have alpha?  - pass in a __tableIndex from a MovieTexture, ImageTexture or PixelTexture. */
int isTextureAlpha(int texno) {
	textureTableIndexStruct_s *ti;

	/* no, have not even started looking at this */
	/* if (texno == 0) return FALSE; */

	ti = getTableIndex(texno);
	if (ti==NULL) return FALSE;

	if (ti->status==TEX_LOADED) {
		return ti->hasAlpha;
	}
	return FALSE;
}


/* is the texture thread initialized yet? */
int fwl_isTextureinitialized() {
	return gglobal()->LoadTextures.TextureThreadInitialized;
}

/* is this texture loaded? used in LoadSensor */
int fwl_isTextureLoaded(int texno) {
	textureTableIndexStruct_s *ti;

	/* no, have not even started looking at this */
	/* if (texno == 0) return FALSE; */

	ti = getTableIndex(texno);
	return (ti->status==TEX_LOADED);
}

/* statusbar uses this to tell user that we are still loading */
int fwl_isTextureParsing() {
	ppTextures p = (ppTextures)gglobal()->Textures.prv;

	/* return currentlyWorkingOn>=0; */
#ifdef TEXVERBOSE 
    if (p->textureInProcess > 0) {
	printf("call to fwl_isTextureParsing %d, returning %d\n",
	       p->textureInProcess,p->textureInProcess > 0);
    }
#endif
	return p->textureInProcess >0;
}

/* this node has changed - if there was a texture, destroy it */
void releaseTexture(struct X3D_Node *node) {

	int tableIndex;
	textureTableIndexStruct_s *ti;

		if (node->_nodeType == NODE_ImageTexture) {
			tableIndex  = ((struct X3D_ImageTexture *)node)->__textureTableIndex;
		} else if (node->_nodeType == NODE_PixelTexture) {
			tableIndex  = ((struct X3D_PixelTexture *)node)->__textureTableIndex;
		} else if (node->_nodeType == NODE_MovieTexture) {
			tableIndex  = ((struct X3D_MovieTexture *)node)->__textureTableIndex;
		} else if (node->_nodeType == NODE_VRML1_Texture2) {
			tableIndex  = ((struct X3D_VRML1_Texture2 *)node)->__textureTableIndex;
		} else return;

#ifdef TEXVERBOSE
	printf ("releaseTexture, calling getTableIndex\n");
	ti = getTableIndex(tableIndex);
	printf ("releaseTexture, ti %p, ti->status %d\n",ti,ti->status);
	ti->status = TEX_NOTLOADED;

	if (ti->OpenGLTexture != TEXTURE_INVALID) {
		printf ("deleting %d textures, starting at %u\n",ti->frames, ti->OpenGLTexture);
		ti->OpenGLTexture = TEXTURE_INVALID;
/* 		FREE_IF_NZ(ti->OpenGLTexture); */
	}
#endif

	ti = getTableIndex(tableIndex);
	ti->status = TEX_NOTLOADED;
	if (ti->OpenGLTexture != TEXTURE_INVALID) {
		FW_GL_DELETETEXTURES(1, &ti->OpenGLTexture);
		ti->OpenGLTexture = TEXTURE_INVALID;
/* 		FREE_IF_NZ(ti->OpenGLTexture); */
	}
}


textureTableIndexStruct_s *getTableIndex(int indx) {
	ppTextures p = (ppTextures)gglobal()->Textures.prv;

#ifdef VERBOSE
{char line[200];
sprintf (line,"getTableIndex, looking for %d",indx);
ConsoleMessage (line);}
#endif
	if (indx < 0) {
		ConsoleMessage ("getTableIndex, texture <0 requested");
		return NULL;
	}

	if (p->activeTextureTable ==NULL ) {
		ConsoleMessage ("NULL sizing errror in getTableIndex");
		return NULL;
	}


	if (indx >= vectorSize(p->activeTextureTable)) {
		ConsoleMessage ("sizing errror in getTableIndex");
		return NULL;
	}


#ifdef VERBOSE
{char line[200];
printf ("getTableIndex - valid request\n");
sprintf (line,"getTableIndex, for %d, size %d",indx, vectorSize(p->activeTextureTable));
ConsoleMessage (line);}
#endif

	return vector_get(textureTableIndexStruct_s *, p->activeTextureTable, indx);
}

/* is this node a texture node? if so, lets keep track of its textures. */
/* worry about threads - do not make anything reallocable */
void registerTexture(struct X3D_Node *tmp) {
	struct X3D_ImageTexture *it;


	it = (struct X3D_ImageTexture *) tmp;
	/* printf ("registerTexture, found a %s\n",stringNodeType(it->_nodeType));  */

	if ((it->_nodeType == NODE_ImageTexture) || (it->_nodeType == NODE_PixelTexture) ||
		(it->_nodeType == NODE_ImageCubeMapTexture) ||
/* JAS - still to implement 
		(it->_nodeType == NODE_GeneratedCubeMapTexture) ||
*/ 
		(it->_nodeType == NODE_MovieTexture) || (it->_nodeType == NODE_VRML1_Texture2)) {

		// for the index, stored in the X3D node.
		int textureNumber;
		ppTextures p;
		// new texture table entry 
		textureTableIndexStruct_s * newTexture = MALLOC (textureTableIndexStruct_s *,sizeof (textureTableIndexStruct_s));
		/* zero the filename so we don't attempt to read dirty data */
		newTexture->filename = NULL;

		p = (ppTextures)gglobal()->Textures.prv;

		if (p->activeTextureTable == NULL) {
			p->activeTextureTable =newVector(textureTableIndexStruct_s *, 16);
		}

		// keep track of which texture this one is.
		textureNumber = vectorSize(p->activeTextureTable);

		//{char line[200]; sprintf (line,"registerTexture textureNumber %d",textureNumber); ConsoleMessage(line);}

		DEBUG_TEX("CREATING TEXTURE NODE: type %d\n", it->_nodeType);
		/* I need to know the texture "url" here... */

		switch (it->_nodeType) {
		/* save this index in the scene graph node */
		case NODE_ImageTexture:
			it->__textureTableIndex = textureNumber;
			break;
		case NODE_PixelTexture: {
			struct X3D_PixelTexture *pt;
			pt = (struct X3D_PixelTexture *) tmp;
			pt->__textureTableIndex = textureNumber;
			break; }
		case NODE_MovieTexture: {
			struct X3D_MovieTexture *mt;
			mt = (struct X3D_MovieTexture *) tmp;
			mt->__textureTableIndex = textureNumber;
			break; }
		case NODE_VRML1_Texture2: {
			struct X3D_VRML1_Texture2 *v1t;
			v1t = (struct X3D_VRML1_Texture2 *) tmp;
			v1t->__textureTableIndex = textureNumber;
			break; }
/* JAS still to implement 
		case NODE_GeneratedCubeMapTexture: {
			struct X3D_GeneratedCubeMapTexture *v1t;
			v1t = (struct X3D_GeneratedCubeMapTexture *) tmp;
			v1t->__textureTableIndex = textureNumber; 
			break;
		}
*/
		case NODE_ImageCubeMapTexture: {
			struct X3D_ImageCubeMapTexture *v1t;
			v1t = (struct X3D_ImageCubeMapTexture *) tmp;
			v1t->__textureTableIndex = textureNumber;
			break;
		}
		}

		/* set the scenegraphNode here */
		newTexture->nodeType = it->_nodeType;
		newTexture->scenegraphNode = X3D_NODE(tmp);

		// save this to our texture table
		vector_pushBack(textureTableIndexStruct_s *, p->activeTextureTable, newTexture);
	} else {
		//ConsoleMessage ("registerTexture, ignoring this node");
	}
}

/* do TextureBackground textures, if possible */
void loadBackgroundTextures (struct X3D_Background *node) {
	struct X3D_ImageTexture *thistex;
	struct X3D_TextureProperties *thistp;
	struct Multi_String thisurl;
	int count;

	/* initialization */
	struct textureVertexInfo mtf = {boxtex,2,GL_FLOAT,0,NULL};
	thisurl.n = 0; thisurl.p = NULL;
	thistex = NULL;

	for (count=0; count<6; count++) {
		/* go through these, back, front, top, bottom, right left */
		switch (count) {
			case 0: {thistex = X3D_IMAGETEXTURE(node->__frontTexture);  thisurl = node->frontUrl; break;}
			case 1: {thistex = X3D_IMAGETEXTURE(node->__backTexture);   thisurl = node->backUrl; break;}
			case 2: {thistex = X3D_IMAGETEXTURE(node->__topTexture);    thisurl = node->topUrl; break;}
			case 3: {thistex = X3D_IMAGETEXTURE(node->__bottomTexture); thisurl = node->bottomUrl; break;}
			case 4: {thistex = X3D_IMAGETEXTURE(node->__rightTexture);  thisurl = node->rightUrl; break;}
			case 5: {thistex = X3D_IMAGETEXTURE(node->__leftTexture);   thisurl = node->leftUrl; break;}
		}
		if (thisurl.n != 0 ) {
			/* we might have to create a "shadow" node for the image texture */
			if (thistex == NULL) {
				int i;
				thistex = createNewX3DNode(NODE_ImageTexture);
				thistp = createNewX3DNode (NODE_TextureProperties);

				/* set up TextureProperties, and link it in */

				/* we use the generic TextureProperties - especially the GenerateMipMaps flag... */
				thistp->generateMipMaps = GL_FALSE; /* default settings, put here to ensure that */
								/* future changes to the spec do no harm */

				thistex->textureProperties = X3D_NODE(thistp);
				ADD_PARENT(X3D_NODE(thistp), X3D_NODE(thistex));	

#ifdef TEXVERBOSE
				printf ("bg, creating shadow texture node url.n = %d\n",thisurl.n);
#endif

				/* copy over the urls */
				thistex->url.p = MALLOC(struct Uni_String **, sizeof (struct Uni_String) * thisurl.n);
				for (i=0; i<thisurl.n; i++) {
					thistex->url.p[i] = newASCIIString (thisurl.p[i]->strptr);
				}
				thistex->url.n = thisurl.n;

				switch (count) {
					case 0: {node->__frontTexture = X3D_NODE(thistex);  break;}
					case 1: {node->__backTexture = X3D_NODE(thistex);   break;}
					case 2: {node->__topTexture = X3D_NODE(thistex);    break;}
					case 3: {node->__bottomTexture = X3D_NODE(thistex); break;}
					case 4: {node->__rightTexture = X3D_NODE(thistex);  break;}
					case 5: {node->__leftTexture = X3D_NODE(thistex);   break;}
				}
			}

			/* we have an image specified for this face */
			gglobal()->RenderFuncs.textureStackTop = 0;
			/* render the proper texture */
			render_node(X3D_NODE(thistex));
		        //OLDCODE FW_GL_COLOR3D(1.0,1.0,1.0);

        		textureDraw_start(&mtf);
        		FW_GL_VERTEX_POINTER(3,GL_FLOAT,0,BackgroundVert);
        		FW_GL_NORMAL_POINTER(GL_FLOAT,0,Backnorms);

        		FW_GL_DRAWARRAYS (GL_TRIANGLES, count*6, 6);
        		textureDraw_end();
		}
	}
}

/* do TextureBackground textures, if possible */
void loadTextureBackgroundTextures (struct X3D_TextureBackground *node) {
	struct X3D_Node *thistex = NULL;
	struct X3D_TextureProperties *thistp = NULL;
	int count;
	struct textureVertexInfo mtf = {boxtex,2,GL_FLOAT,0,NULL};

	for (count=0; count<6; count++) {
		/* go through these, back, front, top, bottom, right left */
		switch (count) {
			case 0: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->frontTexture,thistex);  break;}
			case 1: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->backTexture,thistex);   break;}
			case 2: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->topTexture,thistex);    break;}
			case 3: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->bottomTexture,thistex); break;}
			case 4: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->rightTexture,thistex);  break;}
			case 5: {POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->leftTexture,thistex);   break;}
		}
		if (thistex != 0) {
			/* we have an image specified for this face */
			/* the X3D spec says that a X3DTextureNode has to be one of... */
			if ((thistex->_nodeType == NODE_ImageTexture) ||
			    (thistex->_nodeType == NODE_PixelTexture) ||
			    (thistex->_nodeType == NODE_MovieTexture) ||
			    (thistex->_nodeType == NODE_MultiTexture)) {

				/* if we have no texture properties, add one here to disable mipmapping */
				switch (thistex->_nodeType) {
					case NODE_ImageTexture: {
						if (X3D_IMAGETEXTURE(thistex)->textureProperties == NULL) {
							thistp = createNewX3DNode (NODE_TextureProperties);
							X3D_IMAGETEXTURE(thistex)->textureProperties = X3D_NODE(thistp);
							ADD_PARENT(X3D_NODE(thistp),thistex);
						}
						break;
					}

					case NODE_PixelTexture: {
						if (X3D_PIXELTEXTURE(thistex)->textureProperties == NULL) {
							thistp = createNewX3DNode (NODE_TextureProperties);
							X3D_PIXELTEXTURE(thistex)->textureProperties = X3D_NODE(thistp);
							ADD_PARENT(X3D_NODE(thistp),thistex);
						}
						break;
					}

					case NODE_MovieTexture:
					case NODE_MultiTexture:
					break;
				};

				gglobal()->RenderFuncs.textureStackTop = 0;
				/* render the proper texture */
				render_node((void *)thistex);
		                //OLDCODE FW_GL_COLOR3D(1.0,1.0,1.0);

        			textureDraw_start(&mtf);
        			FW_GL_VERTEX_POINTER(3,GL_FLOAT,0,BackgroundVert);
        			FW_GL_NORMAL_POINTER(GL_FLOAT,0,Backnorms);

        			FW_GL_DRAWARRAYS (GL_TRIANGLES, count*6, 6);
        			textureDraw_end();
			} 
		}
	}
}

/* load in a texture, if possible */
void loadTextureNode (struct X3D_Node *node, struct multiTexParams *param) 
{
    if (NODE_NEEDS_COMPILING) {

	DEBUG_TEX ("FORCE NODE RELOAD: %p %s\n", node, stringNodeType(node->_nodeType));

	/* force a node reload - make it a new texture. Don't change
	   the parameters for the original number, because if this
	   texture is shared, then ALL references will change! so,
	   we just accept that the current texture parameters have to
	   be left behind. */
	MARK_NODE_COMPILED;
	
	/* this will cause bind_image to create a new "slot" for this texture */
	/* cast to GLuint because __texture defined in VRMLNodes.pm as SFInt */
	
	switch (node->_nodeType) {

		case NODE_MovieTexture: {
	    		releaseTexture(node); 
#ifdef HAVE_TO_REIMPLEMENT_MOVIETEXTURES
	    		mym = (struct X3D_MovieTexture *)node;
	    		/*  did the URL's change? we can't test for _change here, because
				movie running will change it, so we look at the urls. */
			    if ((mym->url.p) != (mym->__oldurl.p)) {
				releaseTexture(node); 
				mym->__oldurl.p = mym->url.p;
			    }
#endif
		}
		break;

		case NODE_PixelTexture:
	    		releaseTexture(node); 
		break;

		case NODE_ImageTexture:
	    		releaseTexture(node); 
		break;

		case NODE_VRML1_Texture2:
	    		releaseTexture(node); 
		break;

/* JAS - still to implement
		case NODE_GeneratedCubeMapTexture:
	    		releaseTexture(node); 
		break;

*/
		case NODE_ImageCubeMapTexture:
	    		releaseTexture(node); 
		break;

		default: {
			printf ("loadTextureNode, unknown node type %s\n",stringNodeType(node->_nodeType));
			return;
		}

	    }
	}
    new_bind_image (X3D_NODE(node), param);
	return;
}

static void compileMultiTexture (struct X3D_MultiTexture *node) {
    struct multiTexParams *paramPtr;
    char *param;
    int count;
    int max;
    
    /*  have to regen the shape*/
    MARK_NODE_COMPILED;
    
    /* alloc fields, if required - only do this once, even if node changes */
    if (node->__params == 0) {
        /* printf ("loadMulti, MALLOCing for params\n"); */
        node->__params = MALLOC (void *, sizeof (struct multiTexParams) * gglobal()->display.rdr_caps.texture_units);
        
       // printf ("just mallocd %ld in size for __params\n",sizeof (struct multiTexParams) * gglobal()->display.rdr_caps.texture_units);
    
        
        //printf ("paramPtr is %p\n",(int *)node->__params);
        
        paramPtr = (struct multiTexParams*) node->__params;
        
        /* set defaults for these fields */
        for (count = 0; count < gglobal()->display.rdr_caps.texture_units; count++) {
            paramPtr->multitex_mode= MTMODE_MODULATE;
            paramPtr->multitex_source=INT_ID_UNDEFINED;
            paramPtr->multitex_function=INT_ID_UNDEFINED;
            paramPtr++;
        }
    }
    
    /* how many textures can we use? no sense scanning those we cant use */
    max = node->mode.n; 
    if (max > gglobal()->display.rdr_caps.texture_units) max = gglobal()->display.rdr_caps.texture_units;
    
    // warn users that function and source parameters not looked at right now 
    if ((node->source.n>0) || (node->function.n>0)) {
        ConsoleMessage ("currently, MultiTexture source and function parameters defaults used");
    }
    /* go through the params, and change string name into an int */
    paramPtr = (struct multiTexParams*) node->__params;
    for (count = 0; count < max; count++) {
        param = node->mode.p[count]->strptr;
        paramPtr->multitex_mode = findFieldInMULTITEXTUREMODE(param);
        
        if(node->source.n>count) {
            param = node->source.p[count]->strptr;
            paramPtr->multitex_source = findFieldInMULTITEXTURESOURCE(param);
        }
        
        if (node->function.n>count) {
            param = node->function.p[count]->strptr;
            paramPtr->multitex_function = findFieldInMULTITEXTUREFUNCTION(param);
        }

printf ("compile_MultiTexture, %d of %d, string %s mode %d function %d\n",count,max,param,paramPtr->multitex_mode,paramPtr->multitex_function);

        paramPtr++;
    }
}

void loadMultiTexture (struct X3D_MultiTexture *node) {
	int count;
	int max;
	struct multiTexParams *paramPtr;
	struct X3D_ImageTexture *nt;

#ifdef TEXVERBOSE
	 printf ("loadMultiTexture, this %s has %d textures %x %x\n",stringNodeType(node->_nodeType),
			node->texture.n,
			(int) node->texture.p[0], (int) node->texture.p[1]);
	printf ("	change %d ichange %d\n",node->_change, node->_ichange);
#endif
	
	/* new node, or node paramaters changed */
        if (NODE_NEEDS_COMPILING) {
            compileMultiTexture(node);
	}

	/* ok, normally the scene graph contains function pointers. What we have
	   here is a set of pointers to datastructures of (hopefully!)
	   types like X3D_ImageTexture, X3D_PixelTexture, and X3D_MovieTexture.

	*/

	/* how many textures can we use? */
	max = node->texture.n; 
    //printf ("texture.n %d, texture_units %d, MAX_MULTITEXTURE %d\n", node->texture.n, gglobal()->display.rdr_caps.texture_units, MAX_MULTITEXTURE);
    
	if (max > gglobal()->display.rdr_caps.texture_units) max = gglobal()->display.rdr_caps.texture_units;
    if (max > MAX_MULTITEXTURE) max = MAX_MULTITEXTURE;
    
    
	/* go through and get all of the textures */
	paramPtr = (struct multiTexParams *) node->__params;
    
#ifdef TEXVERBOSE
    printf ("loadMultiTExture, param stack:\n");
    for (count=0; count<max; count++) {
        printf ("   tex %d source %d mode %d\n",count,paramPtr[count].multitex_source,paramPtr[count].multitex_mode);
    }
#endif 

	for (count=0; count < max; count++) {
#ifdef TEXVERBOSE
		printf ("loadMultiTexture, working on texture %d\n",count);
#endif
        
		/* get the texture */
		nt = X3D_IMAGETEXTURE(node->texture.p[count]);

		switch (nt->_nodeType) {
			case NODE_PixelTexture:
			case NODE_ImageTexture : 
				/* printf ("MultiTexture %d is a ImageTexture param %d\n",count,*paramPtr);  */
				loadTextureNode (X3D_NODE(nt),paramPtr);
				break;
			case NODE_MultiTexture:
				printf ("MultiTexture texture %d is a MULTITEXTURE!!\n",count);
				break;
			default:
				printf ("MultiTexture - unknown sub texture type %d\n",
						nt->_nodeType);
		}

		/* now, lets increment textureStackTop. The current texture will be
		   stored in boundTextureStack[textureStackTop]; textureStackTop will be 1
		   for "normal" textures; at least 1 for MultiTextures. */

        	gglobal()->RenderFuncs.textureStackTop++;
 
		
        paramPtr++;

#ifdef TEXVERBOSE
        printf ("loadMultiTexture, textureStackTop %d\n",gglobal()->RenderFuncs.textureStackTop);
		printf ("loadMultiTexture, finished with texture %d\n",count);
#endif
	}
}

#define BOUNDARY_TO_GL(direct) \
				switch (findFieldInTEXTUREBOUNDARYKEYWORDS(tpNode->boundaryMode##direct->strptr)) { \
					case TB_CLAMP: direct##rc=GL_CLAMP; break; \
					case TB_CLAMP_TO_EDGE: direct##rc=GL_CLAMP_TO_EDGE; break; \
					case TB_CLAMP_TO_BOUNDARY: direct##rc=GL_CLAMP_TO_BORDER; break; \
					case TB_MIRRORED_REPEAT: direct##rc=GL_MIRRORED_REPEAT; break; \
					case TB_REPEAT: direct##rc=GL_REPEAT; break; \
					default: direct##rc = GL_REPEAT; \
				}

/* do we do 1 texture, or is this a series of textures, requiring final binding
   by this thread? */
#define DEF_FINDFIELD(arr) \
 static int findFieldIn##arr(const char* field) \
 { \
  return findFieldInARR(field, arr, arr##_COUNT); \
 }

DEF_FINDFIELD(TEXTUREMINIFICATIONKEYWORDS)
DEF_FINDFIELD(TEXTUREMAGNIFICATIONKEYWORDS)
DEF_FINDFIELD(TEXTUREBOUNDARYKEYWORDS)
DEF_FINDFIELD(TEXTURECOMPRESSIONKEYWORDS)



static void move_texture_to_opengl(textureTableIndexStruct_s* me) {
	int rx,ry,sx,sy;
	int x,y;
	GLint iformat;
	GLenum format;

	/* default texture properties; can be changed by a TextureProperties node */
	float anisotropicDegree=1.0f;
	int borderWidth;

        GLint Trc,Src,Rrc;
        GLint minFilter, magFilter;
        GLint compression;
        int generateMipMaps;

	int nurls;

	unsigned char *mytexdata;

	
	/* for getting repeatS and repeatT info. */
	struct X3D_PixelTexture *pt = NULL;
	struct X3D_MovieTexture *mt = NULL;
	struct X3D_ImageTexture *it = NULL;
	struct X3D_VRML1_Texture2* v1t = NULL;
	struct X3D_TextureProperties *tpNode = NULL;
	int haveValidTexturePropertiesNode;
	GLfloat texPri;
	struct SFColorRGBA borderColour;

	/* initialization */
	Src = FALSE; Trc = FALSE; Rrc = FALSE;
	tpNode = NULL;
	haveValidTexturePropertiesNode = FALSE;
	texPri=0.0f;
	borderColour.c[0]=0.0f;borderColour.c[1]=0.0f;borderColour.c[2]=0.0f;borderColour.c[3]=0.0f;
	compression = GL_FALSE;
	borderWidth = 0;
	nurls=1;
	mytexdata = NULL;

    /* did this node get killed on the way here? */
    if (!checkNode(me->scenegraphNode, __FILE__,__LINE__)) {
        ConsoleMessage ("main node disappeared, ignoring texture\n");
        me->status = TEXTURE_INVALID;
        return;
    }
	/* printf ("move_texture_to_opengl, node of type %s\n",stringNodeType(me->scenegraphNode->_nodeType));  */

	/* is this texture invalid and NOT caught before here? */
	/* this is the same as the defaultBlankTexture; the following code should NOT be executed */
	if (me->texdata == NULL) {
		char buff[] = {0x70, 0x70, 0x70, 0xff} ; /* same format as ImageTextures - GL_BGRA or GL_RGBA here */
		me->x = 1;
		me->y = 1;
		me->hasAlpha = FALSE;
		me->texdata = MALLOC(unsigned char *, 4);
		memcpy (me->texdata, buff, 4);
	}

	/* do we need to convert this to an OpenGL texture stream?*/
 
	/* we need to get parameters. */	
	if (me->OpenGLTexture == TEXTURE_INVALID) {
/* 		me->OpenGLTexture = MALLOC (GLuint *, sizeof (GLuint) * me->frames); */
		FW_GL_GENTEXTURES (1, &me->OpenGLTexture);
#ifdef TEXVERBOSE
		printf ("just glGend texture for block %p is %u, type %s\n",
			me, me->OpenGLTexture,stringNodeType(me->nodeType));
#endif

	}

	/* get the repeatS and repeatT info from the scenegraph node */
	if (me->nodeType == NODE_ImageTexture) {
		it = (struct X3D_ImageTexture *) me->scenegraphNode;
		Src = it->repeatS; Trc = it->repeatT;
		tpNode = X3D_TEXTUREPROPERTIES(it->textureProperties);
	} else if (me->nodeType == NODE_PixelTexture) {
		pt = (struct X3D_PixelTexture *) me->scenegraphNode;
		Src = pt->repeatS; Trc = pt->repeatT;
		tpNode = X3D_TEXTUREPROPERTIES(pt->textureProperties);
	} else if (me->nodeType == NODE_MovieTexture) {
		mt = (struct X3D_MovieTexture *) me->scenegraphNode;
		Src = mt->repeatS; Trc = mt->repeatT;
		tpNode =X3D_TEXTUREPROPERTIES(mt->textureProperties);
	} else if (me->nodeType == NODE_ImageCubeMapTexture) {
		struct X3D_ImageCubeMapTexture *mi = (struct X3D_ImageCubeMapTexture *) me->scenegraphNode;
		tpNode = X3D_TEXTUREPROPERTIES(mi->textureProperties);
	} else if (me->nodeType == NODE_VRML1_Texture2) {
		v1t = (struct X3D_VRML1_Texture2 *) me->scenegraphNode;
		Src = v1t->_wrapS==VRML1MOD_REPEAT;
		Trc = v1t->_wrapT==VRML1MOD_REPEAT;
	}



	/* do we have a TextureProperties node? */
	if (tpNode) {
		if (tpNode->_nodeType != NODE_TextureProperties) {
			ConsoleMessage ("have a %s as a textureProperties node",stringNodeType(tpNode->_nodeType));
		} else {
			haveValidTexturePropertiesNode = TRUE;
			generateMipMaps = tpNode->generateMipMaps?GL_TRUE:GL_FALSE;
			texPri = tpNode->texturePriority;
			if ((texPri < 0.0) || (texPri>1.0)) {
				texPri = 0.0f;
				ConsoleMessage ("invalid texturePriority of %f",tpNode->texturePriority);
			}
			memcpy(&borderColour,&(tpNode->borderColor),sizeof(struct SFColorRGBA));

			anisotropicDegree = tpNode->anisotropicDegree;
			if ((anisotropicDegree < 1.0) || (anisotropicDegree>gglobal()->display.rdr_caps.anisotropicDegree)) {
				/* we can be quiet here 
				   ConsoleMessage ("anisotropicDegree error %f, must be between 1.0 and %f",anisotropicDegree, gglobal()->display.rdr_caps.anisotropicDegree);
				*/
				anisotropicDegree = gglobal()->display.rdr_caps.anisotropicDegree;
			}			

			borderWidth = tpNode->borderWidth;
			if (borderWidth < 0) borderWidth=0; if (borderWidth>1) borderWidth = 1;

			switch (findFieldInTEXTUREMAGNIFICATIONKEYWORDS(tpNode->magnificationFilter->strptr)) {
				case TMAG_AVG_PIXEL: magFilter = GL_NEAREST; break;
				case TMAG_DEFAULT: magFilter = GL_LINEAR; break;
				case TMAG_FASTEST: magFilter = GL_LINEAR; break;  /* DEFAULT */
				case TMAG_NEAREST_PIXEL: magFilter = GL_NEAREST; break;
				case TMAG_NICEST: magFilter = GL_NEAREST; break;
				default: magFilter = GL_NEAREST; ConsoleMessage ("unknown magnification filter %s",
					tpNode->magnificationFilter->strptr);
			}

			/* minFilter depends on Mipmapping */
			if (generateMipMaps) switch (findFieldInTEXTUREMINIFICATIONKEYWORDS(tpNode->minificationFilter->strptr)) {
				case TMIN_AVG_PIXEL: minFilter = GL_NEAREST; break;
				case TMIN_AVG_PIXEL_AVG_MIPMAP: minFilter = GL_NEAREST_MIPMAP_NEAREST; break;
				case TMIN_AVG_PIXEL_NEAREST_MIPMAP: minFilter = GL_NEAREST_MIPMAP_NEAREST; break;
				case TMIN_DEFAULT: minFilter = GL_NEAREST_MIPMAP_LINEAR; break;
				case TMIN_FASTEST: minFilter = GL_NEAREST_MIPMAP_LINEAR; break;
				case TMIN_NICEST: minFilter = GL_NEAREST_MIPMAP_NEAREST; break;
				case TMIN_NEAREST_PIXEL: minFilter = GL_NEAREST; break;
				case TMIN_NEAREST_PIXEL_NEAREST_MIPMAP: minFilter = GL_NEAREST_MIPMAP_LINEAR; break;
				default: minFilter = GL_NEAREST_MIPMAP_NEAREST;
					ConsoleMessage ("unknown minificationFilter of %s",
						tpNode->minificationFilter->strptr);
			}
			else switch (findFieldInTEXTUREMINIFICATIONKEYWORDS(tpNode->minificationFilter->strptr)) {
				case TMIN_AVG_PIXEL: 
				case TMIN_AVG_PIXEL_AVG_MIPMAP: 
				case TMIN_AVG_PIXEL_NEAREST_MIPMAP: 
				case TMIN_DEFAULT: 
				case TMIN_FASTEST: 
				case TMIN_NICEST: 
				case TMIN_NEAREST_PIXEL: minFilter = GL_NEAREST; break;
				case TMIN_NEAREST_PIXEL_NEAREST_MIPMAP: minFilter = GL_LINEAR; break;
				default: minFilter = GL_NEAREST;
					ConsoleMessage ("unknown minificationFilter of %s",
						tpNode->minificationFilter->strptr);
			}
					
			switch (findFieldInTEXTURECOMPRESSIONKEYWORDS(tpNode->textureCompression->strptr)) {
				case TC_DEFAULT: compression = GL_FASTEST; break;
				case TC_FASTEST: compression = GL_NONE; break; /* DEFAULT */
				case TC_HIGH: compression = GL_FASTEST; break;
				case TC_LOW: compression = GL_NONE; break;
				case TC_MEDIUM: compression = GL_NICEST; break;
				case TC_NICEST: compression = GL_NICEST; break;

				default: compression = GL_NEAREST_MIPMAP_NEAREST;
					ConsoleMessage ("unknown textureCompression of %s",
						tpNode->textureCompression->strptr);
			}

			BOUNDARY_TO_GL(S);
			BOUNDARY_TO_GL(T);
			BOUNDARY_TO_GL(R);
		}
	} 



	if (!haveValidTexturePropertiesNode) {
		/* convert TRUE/FALSE to GL_TRUE/GL_FALSE for wrapS and wrapT */
		Src = Src ? GL_REPEAT : GL_CLAMP_TO_EDGE; //GL_CLAMP;   //du9 changed from CLAMP to CLAMP_TO_EDGE Sept18,2011 to fix panorama seamline visibility
		Trc = Trc ? GL_REPEAT : GL_CLAMP_TO_EDGE; //GL_CLAMP;
		Rrc = Rrc ? GL_REPEAT : GL_CLAMP_TO_EDGE; //GL_CLAMP;
		generateMipMaps = GL_TRUE;

		if(me->x > 0 && me->y > 0)
		{
			// experimental code to deal with oblong texture images
			// The 'cause' of blurry oblong textures with GLES2 is the anisotropic
			// scaling of the image needed to square up the image for mipmapping. 
			// For example if your texture image is oblong by a factor of 3, then
			// your avatar will need to be 3 times closer to the object to see the
			// same mipmap level as you would with an unscaled image.
			// (OpenGL redbook talks about MipMap levels and rho and lambda.)
			// here we detect if there's big anisotropic scaling/squaring coming up, 
			// and if so turn off mipmapping and squaring
			// scene authors need to make their repeating textures (brick, siding,
			// shingles etc) squarish to get mipmapping and avoid moire/scintilation
			float ratio = 1.0f;
			if(me->x < me->y) ratio = (float)me->y / (float)me->x;
			else ratio = (float)me->x / (float)me->y;
			if(ratio > 2.0f) generateMipMaps = GL_FALSE;
		}

		/* choose smaller images to be NEAREST, larger ones to be LINEAR */
		if ((me->x<=256) || (me->y<=256)) {
			minFilter = GL_NEAREST_MIPMAP_NEAREST;
			if(!generateMipMaps) minFilter = GL_NEAREST;
			magFilter = GL_NEAREST;
		} else {
			minFilter = GL_LINEAR_MIPMAP_NEAREST;
			if(!generateMipMaps) minFilter = GL_LINEAR;
			magFilter = GL_LINEAR;
		}
	}



	/* is this a CubeMap? If so, lets try this... */

	if (getAppearanceProperties()->cubeFace != 0) {
		unsigned char *dest = me->texdata;
        uint32 *sp, *dp;

		int cx;

		#ifndef GL_EXT_texture_cube_map
		# define GL_NORMAL_MAP_EXT                   0x8511
		# define GL_REFLECTION_MAP_EXT               0x8512
		# define GL_TEXTURE_CUBE_MAP_EXT             0x8513
		# define GL_TEXTURE_BINDING_CUBE_MAP_EXT     0x8514
		# define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT  0x8515
		# define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT  0x8516
		# define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT  0x8517
		# define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT  0x8518
		# define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT  0x8519
		# define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT  0x851A
		# define GL_PROXY_TEXTURE_CUBE_MAP_EXT       0x851B
		# define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT    0x851C
		#endif

		#if defined (GL_BGRA)
		iformat = GL_RGBA; format = GL_BGRA;
		#else
		iformat = GL_RGBA; format = GL_RGBA;
		#endif

		/* first image in the ComposedCubeMap, do some setups */
		if (getAppearanceProperties()->cubeFace == GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
			FW_GL_TEXPARAMETERI(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			FW_GL_TEXPARAMETERI(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			FW_GL_TEXPARAMETERI(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			FW_GL_TEXPARAMETERI(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			FW_GL_TEXPARAMETERI(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		}

		rx = me->x;
		ry = me->y;

		/* flip the image around */
		dest = MALLOC (unsigned char *, 4*rx*ry);
		dp = (uint32 *) dest;
		sp = (uint32 *) me->texdata;        



		for (cx=0; cx<rx; cx++) {
			memcpy(&dp[(rx-cx-1)*ry],&sp[cx*ry], ry*4);
		}
	
			myTexImage2D(generateMipMaps, getAppearanceProperties()->cubeFace, 0, iformat,  rx, ry, 0, format, GL_UNSIGNED_BYTE, dest);

		/* last thing to do at the end of the setup for the 6th face */
		if (getAppearanceProperties()->cubeFace == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
			FW_GL_ENABLE(GL_TEXTURE_CUBE_MAP);
			FW_GL_ENABLE(GL_TEXTURE_GEN_S);
			FW_GL_ENABLE(GL_TEXTURE_GEN_T);
			FW_GL_ENABLE(GL_TEXTURE_GEN_R);
		}



	} else {


		/* if we have an ImageCubeMap, we have most likely got a png map; let the
		   render_ImageCubeMapTexture code unpack the maps from this one png */
		if (me->nodeType == NODE_ImageCubeMapTexture) {
			/* this is ok - what is happening is that we have one image, that needs to be 
			   split up into each face */
			/* this should print if we are actually working ok
			if (me->status != TEX_LOADED) {
				printf ("have ImageCubeMapTexture, but status != TEX_LOADED\n");
			}
			*/

			/* call the routine in Component_CubeMapTexturing.c to split this baby apart */
			unpackImageCubeMap(me);
			me->status = TEX_LOADED; /* finito */
		} else {

			/* a pointer to the tex data. We increment the pointer for movie texures */
			mytexdata = me->texdata;
			if (mytexdata == NULL) {
				printf ("mytexdata is null, texture failed, put something here\n");
			}

			FW_GL_BINDTEXTURE (GL_TEXTURE_2D, me->OpenGLTexture);
			
			/* save this to determine whether we need to do material node
			  within appearance or not */
				
			FW_GL_TEXPARAMETERI( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Src);
			FW_GL_TEXPARAMETERI( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Trc);
#if !defined(GL_ES_VERSION_2_0)
			FW_GL_TEXPARAMETERI( GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, Rrc);
			FW_GL_TEXPARAMETERF(GL_TEXTURE_2D,GL_TEXTURE_PRIORITY, texPri);
			FW_GL_TEXPARAMETERFV(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,(GLfloat *)&borderColour);
#endif /*  GL_ES_VERSION_2_0 */

#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
			FW_GL_TEXPARAMETERF(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,anisotropicDegree);
#endif

			if (compression != GL_NONE) {
				FW_GL_TEXPARAMETERI(GL_TEXTURE_2D, GL_TEXTURE_INTERNAL_FORMAT, GL_COMPRESSED_RGBA);
				FW_GL_HINT(GL_TEXTURE_COMPRESSION_HINT, compression);
			}
			x = me->x;
			y = me->y;
		
		
			FW_GL_TEXPARAMETERI( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
			FW_GL_TEXPARAMETERI( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
			
			/* BGRA is seemingly faster on desktop machines... */
			#if defined (GL_BGRA)
			iformat = GL_RGBA; format = GL_BGRA;
			#else
			iformat = GL_RGBA; format = GL_RGBA;
			#endif
			
			/* do the image. */
			if(x && y) {
				unsigned char *dest = mytexdata;
		
				/* do we have to do power of two textures? */
				if (gglobal()->display.rdr_caps.av_npot_texture) {
					rx = x; ry = y;
				} else {
					/* find a power of two that fits */
					rx = 1;
					sx = x;
					while(sx) {sx /= 2; rx *= 2;}
					if(rx/2 == x) {rx /= 2;}
					ry = 1; 
					sy = y;
					while(sy) {sy /= 2; ry *= 2;}
					if(ry/2 == y) {ry /= 2;}
				}
		
				if (gglobal()->internalc.global_print_opengl_errors) {
					DEBUG_MSG("initial texture scale to %d %d\n",rx,ry);
				}
		
				if(rx != x || ry != y || rx > gglobal()->display.rdr_caps.max_texture_size || ry > gglobal()->display.rdr_caps.max_texture_size) {
					/* do we have texture limits??? */
					if (rx > gglobal()->display.rdr_caps.max_texture_size) rx = gglobal()->display.rdr_caps.max_texture_size;
					if (ry > gglobal()->display.rdr_caps.max_texture_size) ry = gglobal()->display.rdr_caps.max_texture_size;
				}
		
				if (gglobal()->internalc.global_print_opengl_errors) {
					DEBUG_MSG("texture size after maxTextureSize taken into account: %d %d, from %d %d\n",rx,ry,x,y);
				}
			

				/* it is a power of 2, lets make sure it is square */
				/* ES 2.0 needs this for cross-platform; do not need to do this for desktops, but
				   lets just keep things consistent 
				   But if not mipmapping, then (experience with win32 GLES2 emulator and QNX device)
				   then it's not necessary to square the image, although current code will get here with
				   generateMipMap always true.
				   */
				if (rx != ry) {
					if(generateMipMaps){
						if (rx>ry)ry=rx;
						else rx=ry;
					}
				}

				/* if scaling is ok... */
				if ((x==rx) && (y==ry)) {
					dest = mytexdata;
				} else {
                    
					/* try this texture on for size, keep scaling down until we can do it */
					/* all textures are 4 bytes/pixel */
					dest = MALLOC(unsigned char *, (unsigned) 4 * rx * ry);

						myScaleImage(x,y,rx,ry,mytexdata,dest);
				}
				
		
				if (gglobal()->internalc.global_print_opengl_errors) {
					DEBUG_MSG("after proxy image stuff, size %d %d\n",rx,ry);
				}
		

				myTexImage2D(generateMipMaps, GL_TEXTURE_2D, 0, iformat,  rx, ry, 0, format, GL_UNSIGNED_BYTE, dest);

				if(mytexdata != dest) {FREE_IF_NZ(dest);}
			}
		
				/* we can get rid of the original texture data here */
				FREE_IF_NZ(me->texdata);
		
		
			FREE_IF_NZ (me->texdata);
		}
	}


	/* ensure this data is written to the driver for the rendering context */
	FW_GL_FLUSH();

	/* and, now, the Texture is loaded */
	me->status = TEX_LOADED;
}

/**********************************************************************************
 bind the image,

	itype 	tells us whether it is a PixelTexture, ImageTexture or MovieTexture.

	parenturl  is a pointer to the url of the parent (for relative loads) OR
		a pointer to the image data (PixelTextures only)

	url	the list of urls from the VRML file, or NULL (for PixelTextures)

	texture_num	the OpenGL texture identifier

	repeatS, repeatT VRML fields

	param - vrml fields, but translated into GL_TEXTURE_ENV_MODE, GL_MODULATE, etc.
************************************************************************************/
void new_bind_image(struct X3D_Node *node, struct multiTexParams *param) {
	int thisTexture;
	int thisTextureType;
	struct X3D_ImageTexture *it;
	struct X3D_PixelTexture *pt;
	struct X3D_MovieTexture *mt;
	struct X3D_ImageCubeMapTexture *ict;
	struct X3D_VRML1_Texture2 *v1t;
/* JAS still to implement
	struct X3D_GeneratedCubeMapTexture *gct;
*/

	textureTableIndexStruct_s *myTableIndex;
	//float dcol[] = {0.8f, 0.8f, 0.8f, 1.0f};
	ppTextures p;
	ttglobal tg = gglobal();
	p = (ppTextures)tg->Textures.prv;

	GET_THIS_TEXTURE;
	myTableIndex = getTableIndex(thisTexture);

	if (myTableIndex->status != TEX_LOADED) {
		DEBUG_TEX("new_bind_image, I am %p, textureStackTop %d, thisTexture is %d myTableIndex %p status %s\n",
		node,tg->RenderFuncs.textureStackTop,thisTexture,myTableIndex, texst(myTableIndex->status));
	}

	/* default here; this is just a blank texture */
	tg->RenderFuncs.boundTextureStack[tg->RenderFuncs.textureStackTop] = tg->Textures.defaultBlankTexture;

	switch (myTableIndex->status) {
		case TEX_NOTLOADED:
			DEBUG_TEX("feeding texture %p to texture thread...\n", myTableIndex);
			myTableIndex->status = TEX_LOADING;
			send_texture_to_loader(myTableIndex);
			break;

		case TEX_LOADING:
			DEBUG_TEX("I've to wait for %p...\n", myTableIndex);
			break;

		case TEX_NEEDSBINDING:
			DEBUG_TEX("texture loaded into memory... now lets load it into OpenGL...\n");
			move_texture_to_opengl(myTableIndex);
			break;

		case TEX_LOADED:
			//DEBUG_TEX("now binding to pre-bound tex %u\n", myTableIndex->OpenGLTexture);
	
			/* set the texture depth - required for Material diffuseColor selection */
			if (myTableIndex->hasAlpha) tg->RenderFuncs.last_texture_type =  TEXTURE_ALPHA;
			else tg->RenderFuncs.last_texture_type = TEXTURE_NO_ALPHA;

//printf ("last_texture_type = TEXTURE_NO_ALPHA now\n"); last_texture_type=TEXTURE_NO_ALPHA;
	
#ifdef HAVE_TO_REIMPLEMENT_MOVIETEXTURES
			if (myTableIndex->nodeType != NODE_MovieTexture) {
#endif
				if (myTableIndex->OpenGLTexture == TEXTURE_INVALID) {
	
					DEBUG_TEX("no openGLtexture here status %s\n", texst(myTableIndex->status));
					return;
				}
	
				tg->RenderFuncs.boundTextureStack[tg->RenderFuncs.textureStackTop] = myTableIndex->OpenGLTexture;
#ifdef HAVE_TO_REIMPLEMENT_MOVIETEXTURES
			} else {
				boundTextureStack[textureStackTop] = 
					((struct X3D_MovieTexture *)myTableIndex->scenegraphNode)->__ctex;
                note that scenegraphNode might be killed; should checkNode for it.
                    
                    
			}
#endif
	
			/* save the texture params for when we go through the MultiTexture stack. Non
			   MultiTextures should have this textureStackTop as 0 */
			 
			if (param != NULL) 
				memcpy(&(tg->RenderTextures.textureParameterStack[tg->RenderFuncs.textureStackTop]), param,sizeof (struct multiTexParams)); 
	
			p->textureInProcess = -1; /* we have finished the whole process */
			break;
			
		case TEX_UNSQUASHED:
		default: {
			printf ("unknown texture status %d\n",myTableIndex->status);
		}
	}
}

#ifdef HAVE_TO_REIMPLEMENT_MOVIETEXTURES
/* FIXME: removed old "really load functions" ... needs to implement loading
          of movie textures.
*/
static void __reallyloadMovieTexture () {

        int x,y,depth,frameCount;
        void *ptr;

        ptr=NULL;

        mpg_main(loadThisTexture->filename, &x,&y,&depth,&frameCount,&ptr);

	#ifdef TEXVERBOSE
	printf ("have x %d y %d depth %d frameCount %d ptr %d\n",x,y,depth,frameCount,ptr);
	#endif

	/* store_tex_info(loadThisTexture, depth, x, y, ptr,depth==4); */

	/* and, manually put the frameCount in. */
	loadThisTexture->frames = frameCount;
}

void getMovieTextureOpenGLFrames(int *highest, int *lowest,int myIndex) {
        textureTableIndexStruct_s *ti;

/*        if (myIndex  == 0) {
		printf ("getMovieTextureOpenGLFrames, myIndex is ZERL\n");
		*highest=0; *lowest=0;
	} else {
*/
	*highest=0; *lowest=0;
	
	#ifdef TEXVERBOSE
	printf ("in getMovieTextureOpenGLFrames, calling getTableIndex\n");
	#endif

       	ti = getTableIndex(myIndex);

/* 	if (ti->frames>0) { */
		if (ti->OpenGLTexture != TEXTURE_INVALID) {
			*lowest = ti->OpenGLTexture;
			*highest = 0;
/* 			*highest = ti->OpenGLTexture[(ti->frames) -1]; */
		}
/* 	} */
}
#endif /* HAVE_TO_REIMPLEMENT_MOVIETEXTURES */
