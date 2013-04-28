/******************************************************************************
 * This file is part of ExcessiveOverkill.                                    *
 *                                                                            *
 * Copyright 2011 Jimmy Bøgh Christensen                                      *
 *                                                                            *
 * ExcessiveOverkill is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * ExcessiveOverkill is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with ExcessiveOverkill.  If not, see <http://www.gnu.org/licenses/>. *
 ******************************************************************************/

#ifndef GUI_H_INCLUDED
#define GUI_H_INCLUDED

#include "types.h"
#include "sprite.h"

void guiInit();
void guiDraw();

guiWindow_s* eoGuiContextCreate(); //Create a non-drawed window to store elements, at 0,0 and with resx*resy size
guiWindow_s* eoGuiContextGet(); //Returns the active context
guiWindow_s* eoGuiAddWindow( guiWindow_s* w, int posx, int posy, int width, int height, const char* title, void* closeBehaviour);
void eoGuiWinBgCol( guiWindow_s* w, GLfloat r, GLfloat g, GLfloat b, GLfloat a );
void eoGuiWinBorCol( guiWindow_s* w, GLfloat r, GLfloat g, GLfloat b, GLfloat a );
void eoGuiBtnSetTxt( guiButton_s* btn, const char* txt );

guiLabel_s* eoGuiAddLabel  (guiWindow_s* container, GLfloat posx, GLfloat posy, const char* text );
guiButton_s* eoGuiAddButton (guiWindow_s* container, GLfloat posx, GLfloat posy, GLfloat width, GLfloat height, const char* text, guiCallback callback );
guiImage_s* eoGuiAddImage  (guiWindow_s* container, GLfloat posx, GLfloat posy, const char* fileName );
guiTextBox_s* eoGuiAddTextBox(guiWindow_s* container, GLfloat posx, GLfloat posy, int numLines, int font, const char* text);

void eoGuiShowCursor( int showCursor );
void eoGuiSetCursor( sprite_s* spr, int pointX, int pointY ); //PointX/Y is offset from 0,0 in the img.
void eoGuiWarpMouse( int16_t x, int16_t y); //Set mouse to desired location.

void eoGuiContextSet( guiWindow_s* container );    //Set as the current active gui container
void eoGuiContextDel( guiWindow_s* container);  //Free all elements in container correctly. Will also set internal activeContainer pointer to 0.

void eoGuiShow();
void eoGuiHide();

int eoGuiFade(int action, int time, guiCallback callback, void* data);


int eoGuiIsActive();

typedef guiWindow_s guiContext;

#endif // GUI_H_INCLUDED
