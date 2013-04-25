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

#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "types.h"

void inputRunKeys();

void inputKeyDown( SDL_keysym key );
void inputKeyUp( SDL_keysym key );

void inputMouseMove( SDL_MouseMotionEvent motion );
void inputMouseButton( SDL_MouseButtonEvent button );
void inputJoyMove( SDL_JoyAxisEvent motion );
void inputJoyButton( SDL_JoyButtonEvent button );
int inputShowBinds(const char* str, void* data);


//Add function as bindable to input system. (can be bound using console command bind, listed using infuncs).
void eoInpAddFunc( const char* funcName, const char* funcDescr, inputCallback cb, int flag );

void eoInpAddHook( int_fast8_t event, int flags, uint16_t key, inputCallback callback );
void eoInpRemHook( int_fast8_t event, uint16_t key, void (*callback)(inputEvent*) );
void inputInit();

#endif // INPUT_H_INCLUDED
