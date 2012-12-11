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

#include "sprite.h"

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "string.h"
#include "console.h"
#include "strings.h"
#include "gfxeng.h"
#include "tick.h"

sprite_base* eoSpriteBaseLoad( const char* fileName )
{
  char line[512], var[512], val[512];
  FILE* file = fopen( fileName, "r" );
  if( !file )
  {
    eoPrint("Couldn't open sprite file: '%s'", fileName );
    return(0);
  }

  sprite_base* b = malloc( sizeof(sprite_base) );
  SDL_Surface* img = 0;
  int cols = 0;
  int rows = 0;

  b->animNumFrames = 1;
  b->animSpeed = 0;



  while(fgets(line, 511, file))
  {
    stripNewLine(line);
    splitVals( '=', line, var, val );

    if(strcmp( var, "file" ) == 0)
    {
      img = IMG_Load( val );
    } else if( strcmp( var, "framex" ) == 0)
    {
      b->frameSize.x = atoi( val );
    } else if( strcmp( var, "framey" ) == 0)
    {
      b->frameSize.y = atoi( val );
    } else if( strcmp( var, "frames" ) == 0)
    {
      b->animNumFrames = atoi( val );
    } else if( strcmp( var, "ticks" ) == 0)
    {
      b->animSpeed = atoi( val );
    } else if( strcmp( var, "rows" ) == 0)
    {
      rows = atoi( val );
    } else if( strcmp( var, "cols" ) == 0)
    {
      cols = atoi( val );
    } else {
      eoPrint("Sprite file '%s', unknown variable '%s' with value %s", fileName, var,val );
    }
  }

  //, int frameTime, int frameSizeX, int frameSizeY,int cols, int rows, int numFrames



  if(!img)
  {
    eoPrint("^2Couldn't load sprite image defined in '^3%s^2'", fileName);
    free(b);
    return(0);
  }

  //We get spritesize from pixel size of a single frame
  b->spriteSize.x = b->frameSize.x/2.0;
  b->spriteSize.y = b->frameSize.y/2.0;

  //We convert frame size to proportionality inside a 1x1 gluint texture.
  b->frameSize.x /= (GLfloat)img->w;
  b->frameSize.y /= (GLfloat)img->h;

  b->tex = eoGfxTexFromSdlSurf( img );
  SDL_FreeSurface( img );



  // Generate lists for frames.
  b->dl = glGenLists( b->animNumFrames );

  int f = 0;
  int row, col;
  GLfloat ustart,ustop,vstart,vstop;

  for( row=0; row<rows; row++ )
  {
    for( col=0; col<cols; col++ )
    {
      ustart = (GLfloat)col*b->frameSize.x;
      ustop  = ((GLfloat)(col+1))*b->frameSize.x;
      vstart = (GLfloat)row*b->frameSize.y;
      vstop  = ((GLfloat)(row+1))*b->frameSize.y;
//      eoPrint("Frame %i uv (%f,%f), (%f,%f)", f,ustart,ustop,vstart,vstop);

      glNewList( b->dl+f, GL_COMPILE );
        glBindTexture( GL_TEXTURE_2D, b->tex );
        glBegin( GL_QUADS );
          glTexCoord2f( ustart,vstart ); glVertex2f( -b->spriteSize.x, -b->spriteSize.y );
          glTexCoord2f( ustop ,vstart ); glVertex2f(  b->spriteSize.x, -b->spriteSize.y );
          glTexCoord2f( ustop ,vstop  ); glVertex2f(  b->spriteSize.x,  b->spriteSize.y );
          glTexCoord2f( ustart,vstop  ); glVertex2f( -b->spriteSize.x,  b->spriteSize.y );
        glEnd();
      glEndList();
      f++;
      //This happens if a column is not filled.
      if(f==b->animNumFrames)
      {
        goto noMoreFrames;
      }
    }
  }
  noMoreFrames:

  return(b);
}

sprite_s* eoSpriteNew( sprite_base* sb, int playing, int looping )
{
  if(!sb)
    return(0);
  sprite_s* s = malloc( sizeof(sprite_s) );
  s->base = sb;
  s->animPlaying = playing;
  s->animLoop = looping;
  s->animFrame = 0;
  s->animTicks = 0;

  eoSpriteScale( s, 1.0, 1.0 );
  return(s);
}

void eoSpriteScale( sprite_s* s, GLfloat scalex, GLfloat scaley )
{
  s->scale.x = scalex;
  s->scale.y = scaley;
}

void spriteDraw( sprite_s* s )
{
  //Macro for performance.
  if( s->scale.x != 0 || s->scale.y != 0 )
  {
    glPushMatrix();
    glScalef( s->scale.x, s->scale.y, 0.0 );
    glCallList( s->base->dl+s->animFrame );
    glPopMatrix();
  } else {
    glCallList( s->base->dl+s->animFrame );
  }

  if( s->animPlaying )
  {
    s->animTicks+=eoTicks();
    if( s->animTicks > s->base->animSpeed )
    {
      s->animTicks=0;
      s->animFrame++;
      if( s->animFrame == s->base->animNumFrames )
      {
        if( s->animLoop )
        {
          s->animFrame=0;
        } else {
          s->animFrame=s->base->animNumFrames-1;
        }
      }
    }
  }
}

void spriteDraw2D( sprite_s* s,vec2 p )
{
  glLoadIdentity();
  glTranslatef( p.x+s->base->spriteSize.x, p.y+s->base->spriteSize.y,0 );
  spriteDraw(s);
}

void eoSpriteDel( sprite_s* s )
{
  free( s );
}

void eoSpriteBaseDel( sprite_base* sb )
{
  engFreeTex( sb->tex );
  free( sb );
}

