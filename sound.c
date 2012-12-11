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

#include "console.h"
#include "sound.h"

#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "gfxeng.h"

typedef struct {
  sound_s* snd;
  int right;
} sndQitm_s;

typedef struct {
  music_s* musPlaying;
  sndQitm_s sndQueue[SND_MAX_QUEUED];
  int inQueue;
  double rightFactor;
} snd_s;

snd_s sndSys;

void sndInit()
{
  sndSys.rightFactor = 255.0/eoSetting()->res.x;

  sndSys.musPlaying = NULL;
  sndSys.inQueue=0;
  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1) {
    eoPrint("sndInit(); Couldn't init SDL_Mixer: '%s' Sound won't work.",Mix_GetError() );
  } else {
    eoPrint("sndInit(); Sound system initialized.");
  }
  Mix_AllocateChannels(SND_MAX_QUEUED);
}

void sndRun()
{
  int i,c=0; //We set C here, so we don't need to loop from start to find next free c.
  if( sndSys.musPlaying && !Mix_PlayingMusic() )
  {
    Mix_FadeInMusic( sndSys.musPlaying->mus, -1, 1000 );
  }

  //Empty queue
  for(i=0; i<sndSys.inQueue; i++)
  {
    //Find free channel
    for( ; c< SND_MAX_QUEUED; c++)
    {
      if( !Mix_Playing(c) )
      {
        //Channel is not playing, set panning
        Mix_SetPanning(c, 255-sndSys.sndQueue[i].right, sndSys.sndQueue[i].right );
        //Break out of for loop and process next item in queue
        Mix_PlayChannel(c, sndSys.sndQueue[i].snd->snd, 0);
        break;
      }
    }

  }
  sndSys.inQueue=0;
}

sound_s* eoSampleLoad( const char* fileName )
{
  Mix_Chunk* chunk = Mix_LoadWAV( fileName );
  if( !chunk )
  {
    eoPrint("eoSampleLoad: Couldn't load '%s'.", fileName);
    return(0);
  }

  sound_s* snd = malloc( sizeof(sound_s) );
  snd->snd = chunk;
  snd->name = malloc( strlen( fileName )+1 );
  strcpy( snd->name, fileName );

  return(snd);
}

music_s* eoMusicLoad( const char* fileName )
{
  Mix_Music* chunk = Mix_LoadMUS( fileName );
  if( !chunk )
  {
    eoPrint("eoMusicLoad: Couldn't load '%s'.", fileName);
    return(0);
  }
  music_s* mus = malloc( sizeof(music_s) );
  mus->mus = chunk;
  mus->name = malloc( strlen(fileName)+1 );
  strcpy( mus->name, fileName );

  return( mus );
}

void eoMusicFadeTo( music_s* mus )
{
  if( sndSys.musPlaying )
  {
    Mix_FadeOutMusic(500);
  }
  sndSys.musPlaying=mus;
}

void eoSamplePlay( sound_s* snd, int right )
{
  //Queue sound to be played at pos.
  if( sndSys.inQueue < SND_MAX_QUEUED )
  {
    sndSys.sndQueue[sndSys.inQueue].snd = snd;
    sndSys.sndQueue[sndSys.inQueue].right = right;
    sndSys.inQueue++;
  } else {
    eoPrint("Sound play queue of %i effects is full, couldn't add sound.", SND_MAX_QUEUED);
  }
}

void eoSampleFree( sound_s* snd )
{
  ///TODO: Remove all queues of this

  Mix_FreeChunk( snd->snd );
  free( snd->name );
  free( snd );
}
void eoMusicDel( music_s* mus )
{
  if( sndSys.musPlaying == mus )
  {
    Mix_HaltMusic();
    sndSys.musPlaying = 0;
  }
  Mix_FreeMusic( mus->mus );
  free( mus->name );
  free( mus );
}

void eoMusicStop(int fadeout)
{
  Mix_FadeOutMusic(fadeout);
  sndSys.musPlaying = 0;
}
