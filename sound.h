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

#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "types.h"

#define SND_MAX_QUEUED 256

void sndInit();
void sndRun();

sound_s* eoSampleLoad( const char* fileName );
music_s* eoMusicLoad( const char* fileName );
void eoMusicFadeTo( music_s* mus ); //Will fade down any currently playing music, and fade new music in.
void eoMusicStop(int fadeout);
void eoSamplePlay( sound_s* snd, int right );
void eoSampleFree( sound_s* snd );
void eoMusicDel( music_s* mus );

#endif // SOUND_H_INCLUDED
