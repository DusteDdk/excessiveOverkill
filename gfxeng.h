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

#ifndef GFXENG_H_INCLUDED
#define GFXENG_H_INCLUDED

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "types.h"

void initGL();
void gfxEngInit();

void engRender(); //Renders the 3D scene

GLuint eoGfxTexFromSdlSurf( SDL_Surface* surf );
GLuint eoGfxLoadTex( const char* fileName );
void engFreeTex( GLuint texName );

renderTex_t* eoGfxFboCreate(int width, int height, bool useDepthBuffer);
void eoGfxFboDel( renderTex_t* rt ); //Frees FBO and texture.
void eoGfxFboRenderBegin( renderTex_t* rt ); //Binds an fbo
void eoGfxFboRenderEnd(); //Unbinds
void eoGfxFboClearTex(); //clears the texture color

void eoGfxBillboardBegin();
void eoGfxBillboard_AxisOnlyBegin(int_fast8_t ax);
void eoGfxBillBoardEnd();

GLfloat eoRandFloat( GLfloat max );
GLfloat eoVec3Len( vec3 v );
vec3 eoVec3Normalize( vec3 v ); //Normalize to unitsize vector
vec3 eoVec3Scale( vec3 v, GLfloat len ); //Scale a vector to len size
vec3 eoVec3FromPoints( vec3 pa, vec3 pb ); //Get an unnormalized directional vector describing the length and direction from pointA to pointB
int eoBestPOT( int i ); //Returns the smalles power of two that is as large or larger than i.
vec3 eoVec3FromAngle( GLfloat radx, GLfloat rady );
vec3 eoVec3Add( vec3 a, vec3 b);
vec2 engRadFromPoints( vec3 a, vec3 b );

settings_t* eoSetting();
#endif // GFXENG_H_INCLUDED
