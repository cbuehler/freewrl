/*
  $Id: MainLoop.h,v 1.15 2011/06/02 19:50:43 dug9 Exp $

  FreeWRL support library.
  UI declarations.

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



#ifndef __FREEWRL_MAINLOOP_MAIN_H__
#define __FREEWRL_MAINLOOP_MAIN_H__

//extern int currentX[20], currentY[20];

void setDisplayed(int);
/* OLDCODE: Now public
void First_ViewPoint();
void Last_ViewPoint();
void Prev_ViewPoint();
void Next_ViewPoint();
*/
void fwl_toggle_headlight();
/* OLDCODE: now in lib header file void RenderSceneUpdateScene(); */

/* should be in OpenGL_Utils.h but this would grab all X3D defs.... */
void setglClearColor(float *val); /* rather start using fwl_set_ClearColor(f,f,f,f) */
/* OLDCODE: now in lib header file int isTextureParsing(); */

/* where this should be ? */
const char* freewrl_get_browser_program();

void resetSensorEvents(void);

#endif /* __FREEWRL_MAINLOOP_MAIN_H__ */
