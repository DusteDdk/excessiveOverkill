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

#ifndef VBOLOAD_H_INCLUDED
#define VBOLOAD_H_INCLUDED

#include "types.h"

//This is only used temporarly untill lists have been compiled.
typedef struct {
  GLfloat specCol[4];   //Specular color
  GLfloat ambDifCol[4];    //Diffuse color
  GLfloat shininess;      //Amount of shininess from 0-128
  GLenum  cubemap;
} matProps;


//Loads an obj file and returns the initialized model. Caller must free. Return 0 on err
vboModel* eoModelLoad( const char* dir, const char* fileName );

//Takes care of freeing resources allocated to model (memory for struct, texture, buffers)
void eoModelFree( vboModel* model );

//returns index of material in model's materialarray, with name supplied, -1 if not found.
int getMaterialIndex(vboModel* model, char* name);

//Draws object geometry
void drawModel( vboModel* model, int_fast8_t fullBright );

//Draws object but in clay without light, env or tex
void drawClayModel( vboModel* model, GLubyte c[3], int_fast8_t fullBright );
void drawWireframeModel( vboModel* model, GLubyte c[4], int_fast8_t fullBright );


#endif // VBOLOAD_H_INCLUDED
