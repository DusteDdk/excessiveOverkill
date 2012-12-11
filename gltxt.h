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

#ifndef GLTXT_H_INCLUDED
#define GLTXT_H_INCLUDED

#include "types.h"

//Initialize fonts.
void gltxtInit(int h);
//Print string to screen, must be called when in ortho.
void eoTxtWrite(int_fast8_t font, int_fast8_t pos, const char* txt, GLfloat x, GLfloat y);
//Convinience function to render a font with a black shadow beneath
void eoTxtWriteShadow(int_fast8_t font, int_fast8_t pos, const char* txt, GLfloat x, GLfloat y);

//For width and size.
GLfloat eoTxtLineWidth( int_fast8_t font, const char* txt, int len );
GLfloat eoTxtHeight( int_fast8_t font );

#endif // GLTXT_H_INCLUDED
