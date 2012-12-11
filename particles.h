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

#ifndef PARTICLES_H_INCLUDED
#define PARTICLES_H_INCLUDED

#include "types.h"
#include "sprite.h"

particleEmitter_s* eoPsysNewEmitter(); //Creates a standard emitter structure to be setup.
void eoPsysBake( particleEmitter_s* emitter ); //Initializes system and adds to list of systems.
void eoPsysEmit( particleEmitter_s* emitter, vec3 pos ); //Single emission, convinience function
void eoPsysFree( particleEmitter_s* emitter );  //Free's data and removes list from system
void psysSim(); //Simulate particles
void psysDraw(); //Draw systems on screen
void psysInit(); //Load default texture, setup list

#endif // PARTICLES_H_INCLUDED
