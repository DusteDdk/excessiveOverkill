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

#include "data.h"
#include "tick.h"
#include "particles.h"
#include "list.h"
#include "gfxeng.h"
#include "console.h"
#include <math.h>
typedef struct {
  sprite_base* defaultSpriteBase;
  listItem* emitters;
} psys_s;

psys_s psys;

//Creates a standard emitter structure to be setup.
particleEmitter_s* eoPsysNewEmitter()
{
  particleEmitter_s* e = malloc( sizeof(particleEmitter_s) );
  memset( (void*)e, 0, sizeof(particleEmitter_s) );

  //Set default texture
  e->sprBase = psys.defaultSpriteBase;

  e->percentFlicker = 100;
  e->particleLifeMax = 1000;
  e->emitType = PAR_EMIT_EXPLO;
  e->sizeMax = 1;
  e->color[0] = 1.0;
  e->color[1] = 1.0;
  e->color[2] = 1.0;
  e->color[3] = 1.0;
  e->colorVariance[0] = 0.0;
  e->colorVariance[1] = 0.0;
  e->colorVariance[2] = 0.0;
  e->colorVariance[3] = 0.0;

  e->rotateParticles = 1;
  return(e);
}

void eoPsysBake( particleEmitter_s* e )
{
  if( e->numParticlesPerEmission && !e->_particles )
  {

    //Calculate max particles
    e->_maxParticles = (int)ceil(e->numParticlesPerEmission*(e->particleLifeMax)/((e->ticksBetweenEmissions-e->emitTimeVariance)?e->ticksBetweenEmissions -e->emitTimeVariance:1));
    e->_availParticles = e->_maxParticles;

    //Allocate array
    e->_particles = malloc( sizeof(particle_s)*e->_maxParticles );
    //Set all null for first run
    memset( (void*)e->_particles, 0, sizeof(particle_s)*e->_maxParticles );
    int i;
    for(i=0; i<e->_maxParticles; i++)
    {
      particle_s* p = &e->_particles[i];
      p->sprite = eoSpriteNew( e->sprBase,1,1 );

    }

    //Add to list of emitters
    listAddData(psys.emitters, (void*)e);

  } else {
    eoPrint("Particle system %p either incomplete or allready baked.");
  }
}

void eoPsysFree( particleEmitter_s* e )
{
  //Remove from list of emitters
  listRemoveByData( psys.emitters, (void*)e);

  int i;

  for(i=0; i<e->_maxParticles; i++)
  {
    eoSpriteDel( e->_particles[i].sprite );
  }

  //Free particles
  if( e->_particles )
    free( e->_particles );

  //Free emitter structure
  free(e);

}

void _eoPsysEmit( particleEmitter_s* e )
{
  int i;
  int emitted=0;
  particle_s* p;

  if( e->_availParticles < e->numParticlesPerEmission )
    eoPrint("_eoPsysEmit(); Error: Supposed to emit %i but only %i free slots in %i size array.", e->numParticlesPerEmission, e->_availParticles, e->_maxParticles );

  GLfloat degY=0,len=0;

  //Adjust set new emitTime
  if( e->ticksBetweenEmissions > 0)
    e->_timeToNextEmit = e->ticksBetweenEmissions-((e->emitTimeVariance)?(rand()%(e->emitTimeVariance)):0 );


  for(i=0; i<e->_maxParticles; i++)
  {
    p = &e->_particles[i];

    if(p->lifeLeft<1)
    {
      //Scaling
      GLfloat s=e->sizeMax - eoRandFloat(e->sizeVariance);
      eoSpriteScale( p->sprite, s,s );
      p->size = s;
      //Coloring
      p->color[0] = e->color[0] - ( eoRandFloat( e->colorVariance[0] ) );
      p->color[1] = e->color[1] - ( eoRandFloat( e->colorVariance[1] ) );
      p->color[2] = e->color[2] - ( eoRandFloat( e->colorVariance[2] ) );
      p->color[3] = e->color[3];
      //Life
      p->lifeLeft = e->particleLifeMax - ( (int)eoRandFloat(e->particleLifeVariance) );
      p->life = p->lifeLeft;
      //Position
      p->position[0] = e->position.x;
      p->position[1] = e->position.y;
      p->position[2] = e->position.z;

      //Set direction
      switch( e->emitType )
      {
        case PAR_EMIT_CONE:

        break;
        case PAR_EMIT_EXPLO:
          p->velocity[0] = (1-eoRandFloat(2))*(e->emitSpeedMax-eoRandFloat(e->emitSpeedVariance));
          p->velocity[1] = (1-eoRandFloat(2))*(e->emitSpeedMax-eoRandFloat(e->emitSpeedVariance));
          p->velocity[2 ] = (1-eoRandFloat(2))*(e->emitSpeedMax-eoRandFloat(e->emitSpeedVariance));
        break;
        case PAR_EMIT_LINE:
        {
          GLfloat step =e->emitSpeedMax/e->numParticlesPerEmission;
          //Speed tells the length of our line
          //Angle tells us the direction of the line
          p->position[0] += cos(e->emitAngle)*len;
          p->position[2] += sin(e->emitAngle)*len;

          //Increase length one particle step
          len += step;

          //Direction tells us the velocity of the particles
          p->velocity[0] = e->emitDirection.x;
          p->velocity[1] = e->emitDirection.y;
          p->velocity[2] = e->emitDirection.z;

        }
        break;
        case PAR_EMIT_RING:
        {
          //There are numParPerEmis steps, a steps is 360/numParEmis
          GLfloat step =360.0/(GLfloat)e->numParticlesPerEmission;

          //Main goal is velocity change
          p->velocity[0] = cos(degY)*(e->emitSpeedMax-eoRandFloat(e->emitSpeedVariance));
          p->velocity[2] = sin(degY)*(e->emitSpeedMax-eoRandFloat(e->emitSpeedVariance));

          //Secondary, we set a starting pos, using angle as the distance
          p->position[0] += cos(degY)*e->emitAngle;
          p->position[2] += sin(degY)*e->emitAngle;

          //Increase angle
          degY += step;
        }
        break;
        case PAR_EMIT_SPHERE:
          eoPrint("Particle system emitter type: Sphere. Not implemented.");
        break;
        default:
          eoPrint("Particle system %p has invalid emission type.", e);
        break;
      }

      //Update emitted and available
      e->_availParticles--;
      emitted++;
      if( emitted == e->numParticlesPerEmission )
        return;

    }
  }


}

void eoPsysEmit( particleEmitter_s* emitter, vec3 pos )
{
  if( emitter->_particles )
  {
    emitter->position = pos;
    emitter->ticksBetweenEmissions = 0;
    _eoPsysEmit(emitter);
  } else {
    eoPrint("eoPsysEmit(); Emitter not baked.");
  }
}

void _psysSimParticles( particleEmitter_s* e)
{
  int i;
  particle_s* p;
  for(i=0; i<e->_maxParticles;i++)
  {

    p = &e->_particles[i];
    if( p->lifeLeft > 0 )
    {
      p->lifeLeft -= eoTicks();

      if( p->lifeLeft < 1 )
        e->_availParticles++;

      p->position[0] += (p->velocity[0]+e->wind.x) / eoTicks();
      p->position[1] += (p->velocity[1]+e->wind.y) / eoTicks();
      p->position[2] += (p->velocity[2]+e->wind.z) / eoTicks();

    }
  }
}

void psysSim()
{
  //Loop through emitters
  listItem* it=psys.emitters;
  particleEmitter_s* e;

  while( (it=it->next) )
  {
    e = (particleEmitter_s*)it->data;
    //Simulate particles in system only if there's any active.
    if( e->_availParticles<e->_maxParticles)
      _psysSimParticles(e);

    //Emit new?
    if( e->ticksBetweenEmissions )
    {
      e->_timeToNextEmit -= eoTicks();
      if( e->_timeToNextEmit < 1 )
      {
        _eoPsysEmit(e);
      }
    }
  }
}

void psysDraw()
{
  int i;
  listItem* it = psys.emitters;
  particleEmitter_s* e;
  particle_s* p;
  GLfloat particleStrength;


  glDepthMask( GL_FALSE );

  glDisable( GL_LIGHTING );

  glEnable(GL_TEXTURE_2D);
  while( (it=it->next) )
  {
    e = (particleEmitter_s*)it->data;

    //Set mode
    if(e->addictive)
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    //Set system texture
    glBindTexture(GL_TEXTURE_2D, e->sprBase->tex );
    //Draw particles
    if( e->_availParticles<e->_maxParticles)
    for(i=0; i<e->_maxParticles; i++)
    {
      p = &e->_particles[i];
      if(p->lifeLeft>0)
      {
       //Flicker? (The first condition avoids calling rand and mod if we're always "on")
        if( (e->percentFlicker==100) || (e->percentFlicker > (rand()%100)) )
        {
          //Used for alpha and size changes (if relevant)
          particleStrength=(GLfloat)p->lifeLeft/(GLfloat)p->life;
          if( e->fade )
            p->color[3] = particleStrength;

            //Set color+alpha
            glColor4fv( p->color );
            glPushMatrix();
            glTranslatef( p->position[0],p->position[1],p->position[2] );
            eoGfxBillboardBegin();

            if( e->shrink )
              eoSpriteScale(p->sprite, particleStrength*p->size, particleStrength*p->size );

            spriteDraw( p->sprite );
            eoGfxBillBoardEnd();
            glPopMatrix();

        }
      }
    }

  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable( GL_LIGHTING );

  glDepthMask( GL_TRUE );

}

void psysInit()
{
  eoPrint("psysInit();");
  psys.defaultSpriteBase = eoSpriteBaseLoad( Data("/data/gfx/","defaultparticle.spr") );
  psys.emitters = initList();
}
