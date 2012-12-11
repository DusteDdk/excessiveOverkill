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

#ifndef SPRITE_H_INCLUDED
#define SPRITE_H_INCLUDED

#include "types.h"

sprite_base* eoSpriteBaseLoad( const char* fileName );
sprite_s* eoSpriteNew( sprite_base* sb, int playing, int looping );
void eoSpriteScale( sprite_s* s, GLfloat scalex, GLfloat scaley );
void eoSpriteDel( sprite_s* s );
void eoSpriteBaseDel( sprite_base* sb );


void spriteDraw( sprite_s* s ); //Really fast but won't translate
void spriteDraw2D( sprite_s* s, vec2 p ); //Fast too, but translates

#endif // SPRITE_H_INCLUDED
