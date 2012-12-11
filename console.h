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

#ifndef CONSOLE_H_INCLUDED
#define CONSOLE_H_INCLUDED

#include <stdarg.h>
#include "types.h"
void consoleInit();
void eoPrint(const char* format, ...);
void consoleRender();
void eoExec( const char* str ); //Parse command string
void eoVarAdd( int type, int len, void* data, const char* name );
//Calls callback with argument as string, and data*, or if no argument, NULL pointer as argument.
void eoFuncAdd( int(*callback)(const char*, void*),void* data, const char* name );
bool consoleVisible(); //Wether console is visible

void _consoleToggle(inputEvent* e);

#endif // CONSOLE_H_INCLUDED
