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

#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include "types.h"
#include "vboload.h"
#include "sprite.h"
#include "gfxeng.h"
#include "sound.h"
#include "particles.h"
#include "eng.h"

typedef struct {
  int isServer;
  int isPaused;

  //Somewhere, there's a world state.
  world_s world; //The world
  int nextObj;
  listItem* _deleteObjs;
  int drawHitbox;
} gameState_s;



void eoGameEnableMouseSelection(GLfloat scale);
void eoDisableMouseSelection();
void eoGameInit();
void eoWorldClear();

void eoRegisterSimFunc( eoEngObjSimCallback );
void eoRegisterStartFrameFunc( void (*startFrameFunc)(void) );

engObj_s* eoEngFind2dObj( vec2 p );
engObj_s* eoObjCreate(int type);
void eoObjBake(engObj_s* obj);
void eoObjAdd(engObj_s* obj);
void eoObjDel(engObj_s* obj);
void eoObjAttach( engObj_s* parent, engObj_s* child );

void gameRun();
gameState_s* eoGetState();
void gameSimMovement();
void gameSimWorld();
void gameDraw();
void gamePlaySounds();

int eoPauseGet();

void eoPauseSet(int p);
void eoRegisterSimFunc( void (*objSimFunc)(engObj_s*) );
void eoRegisterStartFrameFunc( void (*startFrameFunc)(void) );

/*
#define GAME_TIMER_REPEAT
#define GAME_TIMER_ONCE
typedef void (*gameTimerCallback_f)(void*);
void gameAddTimer( int repeat, int interval, void* data, gameTimerCallback_f );
gameAddTimer( GAME_TIMER_ONCE, 30000, NULL, _shipStartFly );
gameAddTimer( GAME_TIMER_ONCE, 35000, NULL, _cameraLockOn );
*/
#endif // GAME_H_INCLUDED
