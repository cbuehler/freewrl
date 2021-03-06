/* 
Vector.h
General purpose containers - vector and stack (implemented on top of it)
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


#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

/* ************************************************************************** */
/* ******************************** Vector ********************************** */
/* ************************************************************************** */

/* This is the vector structure. */
struct Vector
{
 int	n;
 int	allocn;
 void*	data;
};

/* Constructor/destructor */
struct Vector* newVector_(int elSize, int initSize,char *,int);
#define newVector(type, initSize) \
 newVector_((int)sizeof(type), initSize,__FILE__,__LINE__)

#ifdef DEBUG_MALLOC
	void deleteVector_(char *file, int line, int elSize, struct Vector**);
	#define deleteVector(type, me) deleteVector_(__FILE__,__LINE__,(int)sizeof(type), &(me))
#else
	void deleteVector_(int elSize, struct Vector**);
	#define deleteVector(type, me) deleteVector_((int)sizeof(type), &(me))
#endif

/* Ensures there's at least one space free. */
void vector_ensureSpace_(int, struct Vector*);

/* Element retrieval. */
#define vector_get(type, me, ind) \
 ((type*)((struct Vector*)me)->data)[ind]

/* Element set. */
#define vector_set(type,me,element,value) \
        ((type*)((struct Vector *)  (me))->data)[element]=value

/* Size of vector */
#define vectorSize(me) \
 (((struct Vector*)me)->n)

/* Back of a vector */
#define vector_back(type, me) \
 vector_get(type, me, vectorSize(me)-1)

/* Is the vector empty? */
#define vector_empty(me) \
 (!vectorSize(me))

/* Shrink the vector to minimum required space. */
void vector_shrink_(int, struct Vector*);
#define vector_shrink(type, me) \
 vector_shrink_((int)sizeof(type), me)

/* Push back operation. */
#define vector_pushBack(type, me, el) \
 { \
  vector_ensureSpace_((int)sizeof(type), me); \
  ASSERT(((struct Vector*)me)->n<((struct Vector*)me)->allocn); \
  vector_get(type, me, ((struct Vector*)me)->n)=el; \
  ++((struct Vector*)me)->n; \
 }

/* Pop back operation */
#define vector_popBack(type, me) \
 { \
  ASSERT(!vector_empty(me)); \
  --((struct Vector*)me)->n; \
 }
#define vector_popBackN(type, me, popn) \
 { \
  ASSERT(popn<=vectorSize(me)); \
  ((struct Vector*)me)->n-=popn; \
 }

/* Release and get vector data. */
void* vector_releaseData_(int, struct Vector*);
#define vector_releaseData(type, me) \
 vector_releaseData_((int)sizeof(type), me)

/* ************************************************************************** */
/* ************************************ Stack ******************************* */
/* ************************************************************************** */

/* A stack is essentially a vector */
typedef struct Vector Stack;

/* Constructor and destructor */
#define newStack(type) \
 newVector((int) sizeof(type), 4)
#define deleteStack(type, me) \
 deleteVector((int) sizeof(type), me)

/* Push and pop */
#define stack_push(type, me, el) \
 vector_pushBack(type, me, el)
#define stack_pop(type, me) \
 vector_popBack(type, me)

/* Top of stack */
#define stack_top(type, me) \
 vector_get(type, me, vectorSize(me)-1)

/* Is the stack empty? */
#define stack_empty(me) \
 vector_empty(me)

/* tie assert in here to give better failure methodology */
/* #define ASSERT(cond) if(!(cond)){fw_assert(__FILE__,__LINE__);} */
/* void fw_assert(char *,int); */

#endif /* Once-check */
