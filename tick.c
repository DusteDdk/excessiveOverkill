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

#include "tick.h"
#include <SDL/SDL.h>

typedef struct {
  float fps;
  int ticks;
  int logicTicks;
  uint32_t lastTick;
} tickInfo_s;

static tickInfo_s ti;

void eoTicksReset() {
  ti.lastTick = SDL_GetTicks();
  ti.ticks=0;
}

int eoTicks()
{
  return( ti.ticks );
}

int ticksLogic()
{
  return( ti.logicTicks );
}

void tickStartFrame()
{
  uint32_t t = SDL_GetTicks();
  ti.ticks = t-ti.lastTick;
  ti.lastTick=t;
}

void tickStopLogic()
{
  ti.logicTicks = SDL_GetTicks()-ti.lastTick;
}
