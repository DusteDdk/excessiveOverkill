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

#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "types.h"
#include "input.h"

void camInit();
void camBegin(); //Called before drawing anything, sets up modelView matrix

void eoCamPosSet(vec3 p); //Moves the camera to position (camera looks in same direction)
void eoCamTargetSet(vec3 p); //Point camera at position
void eoCamZoomSet( GLfloat zoom ); //1 = normal, 2 = double zoom,

void eoCamMoveForward( GLfloat l );
void eoCamMoveBackward( GLfloat l );
void eoCamMoveLeft( GLfloat l );
void eoCamMoveRight( GLfloat l );
void eoCamMoveUp( GLfloat l );
void eoCamMoveDown( GLfloat l );

vec3 eoCamPosGet();
vec3 eoCamTargetGet();
GLfloat eoCamZoomGet();
vec3 eoCamDirectionGet();
vec2 eoCamRotGet();

void eoCamRecPlay( const char* fileName, int absolute, void (*finishedCallback)() );
void eoCamRecStop();
int cameraBeginRecord( const char* args, void* data );
int cameraEndRecord( const char* args, void* data );
int camConPlayRec( const char* args, void* data );
int cameraRecordRelative( const char* args, void* data);
int eoCamGetPlaybackState();

camData* camGet(); //Returns camera data
int cameraFreeLook( const char* args, void* data ); //Console callback, for enabling freelook

int cameraSetSens( const char* args, void* data ); //Console callback for setting mouse sens

int cameraGrabCursor( const char* args, void* data);
int cameraLockLook( const char* args, void* data);
void camSetMatrix();
void setCameraLockLook(int l);

#endif // CAMERA_H_INCLUDED
