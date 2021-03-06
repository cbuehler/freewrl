/*
  $Id: list.h,v 1.4 2010/08/19 02:20:36 crc_canada Exp $

  FreeWRL support library.
  Linked lists.

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


#ifndef __LIBFREEWRL_LIST_H__
#define __LIBFREEWRL_LIST_H__


////////////////////////////////////////////////////////////////////////////////////////////
// LIST
////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _s_list_t {

    void *elem;
    struct _s_list_t *next;

} s_list_t;

#define ml_elem(_item) (_item->elem)
#define ml_next(_item) (_item->next)

typedef void f_free_t(void *ptr);

extern s_list_t* ml_new(const void *elem);
extern int       ml_count(s_list_t *list);
extern s_list_t* ml_prev(s_list_t *list, s_list_t *item);
extern s_list_t* ml_last(s_list_t *list);
extern s_list_t* ml_find(s_list_t *list, s_list_t *item);
extern s_list_t* ml_find_elem(s_list_t *list, void *elem);
extern s_list_t* ml_insert(s_list_t *list, s_list_t *point, s_list_t *item);
extern s_list_t* ml_append(s_list_t *list, s_list_t *item);
extern void      ml_delete(s_list_t *list, s_list_t *item);
extern s_list_t* ml_delete_self(s_list_t *list, s_list_t *item);
extern void      ml_delete2(s_list_t *list, s_list_t *item, f_free_t f);
extern void      ml_delete_all(s_list_t *list);
extern void      ml_delete_all2(s_list_t *list, f_free_t f);
extern s_list_t* ml_get(s_list_t *list, int index);

#define ml_foreach(_list,_action) {\
					s_list_t *__l;\
					s_list_t *next;\
					for(__l=_list;__l!=NULL;) {\
						next = ml_next(__l); /* we need to get next from __l before action deletes element */ \
						_action;\
						__l = next; \
					}\
				  }
extern void ml_dump(s_list_t *list);
extern void ml_dump_char(s_list_t *list);


#endif /* __LIBFREEWRL_LIST_H__ */
