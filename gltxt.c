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

#include "gltxt.h"
#include "console.h"
#include "data.h"
#include "gfxeng.h"

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <math.h>

static struct {
  GLuint texture;
  GLuint dlBase; //Base name of displaylists
  GLfloat height;

  struct {
    GLfloat width;  //Size of character (width x height)
    vec2 pos;  //Offset on texture
  } chars[256];
} fonts[NUM_FONTS];

//Black, White, Red, Green, Blue, Yellow, Purple, Cyan, semiBlack, semiWhite
static const GLfloat txtColors[10][4] = { { 0.0,0.0,0.0,1.0 }, { 1.0,1.0,1.0,1.0 }, {1.0,0.0,0.0,1.0 }, {0.0,1.0,0.0,1.0 }, {0.0,0.0,1.0,1.0}, {1.0,1.0,0.0,1.0}, { 1.0,0.0,1.0,1.0 }, { 0.0,1.0,1.0,1.0 }, {0.0,0.0,0.0,0.5}, {1.0,1.0,1.0,0.5} };

void gltxtGenFont(const char* fileName, int_fast8_t f,int pointSize, float s)
{
  eoPrint("Loading font[%i]: %s at size: %i", f,fileName, pointSize);
  TTF_Font* font;
  if( !(font=TTF_OpenFont(fileName, round((float)pointSize*s)) ) )
  {
    eoPrint("Couldn't open font %s: %s", fileName, TTF_GetError());
  }
  //Set height of font (in pixels)
  fonts[f].height= (GLfloat)TTF_FontHeight(font);
  eoPrint("Font %i height: %f", f,fonts[f].height );

  //Worst case would be a (pointSize*256) pixels long texture, we square that and chose the smallest pot texture that it fits in.
  int texSize = eoBestPOT( (int)(sqrt(256)*fonts[f].height*s) );


//  eoPrint("Font[%i] texSize: %i Pointsize: %i (Scale %f) Height %f", f, texSize, pointSize,s,fonts[f].height);

  Uint32 rmask, gmask, bmask, amask;
  //Create new sdl surface image for texture
  #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
  #else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
  #endif


  SDL_Surface* texData = SDL_CreateRGBSurface(SDL_SWSURFACE, texSize, texSize, 32,  rmask,gmask,bmask,amask);

  //Blitting destination position
  SDL_Rect r;
  r.x=2;
  r.y=2;


  //We render everything in white and use opengl colors.
  SDL_Color color = {255,255,255};
  SDL_Surface* charTex;
  //Renderer takes a 0 terminated string
  char str[2] = {0,0};

  int charWidth = 0; //Height never actually used.

  //Generate list name
  fonts[f].dlBase = glGenLists(256);

  GLfloat texScale = (1.0/texSize);
  //Let's render some characters (and control codes for good measure!)
  int i;
  for(i=0; i<256; i++)
  {
    str[0] = i;
    charTex = TTF_RenderText_Blended(font, str, color);

    if( charTex )
    {

      SDL_SetAlpha( charTex,0,0 );
      TTF_SizeText( font, str, &charWidth, 0 );
      //Since sqrt of 256 is 16, if we hit %16==0, we switched line
      if( i%16==0 )
      {
        r.x =2;
        r.y += fonts[f].height+2;
      }
      fonts[f].chars[i].width = (GLfloat)charWidth;
      fonts[f].chars[i].pos.x = r.x;
      fonts[f].chars[i].pos.y = r.y;
      SDL_BlitSurface( charTex, 0, texData, &r );
      //Add character width to blit position + 1 (to make a small space to avoid bleeding)
      r.x += charWidth + 2;

      //Generate a displaylist this character (texture is bound first by drawing function to save texture swaps)
      glNewList( fonts[f].dlBase+i, GL_COMPILE );
        glBegin(GL_QUADS);
          glTexCoord2f( texScale*fonts[f].chars[i].pos.x, texScale*fonts[f].chars[i].pos.y );
          glVertex2f( 0, 0 );

          glTexCoord2f( texScale*(fonts[f].chars[i].pos.x+fonts[f].chars[i].width), texScale*fonts[f].chars[i].pos.y );
          glVertex2f( fonts[f].chars[i].width, 0 );

          glTexCoord2f( texScale*(fonts[f].chars[i].pos.x+fonts[f].chars[i].width), texScale*(fonts[f].chars[i].pos.y+fonts[f].height) );
          glVertex2f( fonts[f].chars[i].width, fonts[f].height );

          glTexCoord2f( texScale*(fonts[f].chars[i].pos.x), texScale*(fonts[f].chars[i].pos.y+fonts[f].height) );
          glVertex2f( 0, fonts[f].height );
        glEnd();

        //Translate to next character.
        glTranslatef( fonts[f].chars[i].width,0,0 );
      glEndList();

      SDL_FreeSurface( charTex );
    }
  }

  //Upload texture to OpenGL
  fonts[f].texture = eoGfxTexFromSdlSurf(texData);

  //Free font
  TTF_CloseFont(font);

  //Free SDL surface
  SDL_FreeSurface( texData );

}

//Initializes text rendering system with fonts for an h pixels high display size.
void gltxtInit(int h)
{
  if(TTF_Init()==-1) {
      eoPrint("SDL: TTF_Init: %s", TTF_GetError());
  }

  //We scale fonts so that they fit to a 800x600 glunits 2D interface
  gltxtGenFont( Data("/data/fonts/",FONT_SYS_NAME), FONT_SYS , 8, 1.0 );


  gltxtGenFont( Data("/data/fonts/",FONT_S_NAME), FONT_SMALL , 10, 1.0 );
  gltxtGenFont( Data("/data/fonts/",FONT_M_NAME), FONT_MEDIUM , 18, 1.0 );
  gltxtGenFont( Data("/data/fonts/",FONT_L_NAME), FONT_LARGE , 24, 1.0 );

}

//Check for ^, if it's there, if there's room for a chracter after the color char, then calculate the int value (||num==0) is to always make this true
//Then, if the number is a valid color code, skip the char
bool _controlPair( const char* txt, int len, int i )
{
  int num;
  return( ((txt[i]=='^') && (i+2<len) && ((num=txt[i+1]-48)||num==0) && ( num > -1 && num < 10  ))?num:-1 );
}

inline GLfloat eoTxtLineWidth( int_fast8_t font, const char* txt, int len )
{
  GLfloat lineWidth=0;
  int i; //For converting from ascii to numerical values.
  for(i=0; i<len;i++)
  {
    if( _controlPair( txt, len, i ) != -1 )
      i+=2;

    lineWidth += fonts[font].chars[ (int)txt[i] ].width;
  }
  return( lineWidth );
}

void _gltxt(bool ignoreColors,int_fast8_t font, int_fast8_t pos, const char* txt, GLfloat x, GLfloat y)
{
  int i;
  int len = strlen(txt); //Length is used a minimum of 1 times, so we waste some memory but won't have to call it again.
  int lines=0; //Lines written so far, used for newlines.

  //We save a branch by not doing a case for left align since it's default.
  if(pos != TXT_LEFT)
  {
    switch(pos)
    {
      case TXT_CENTER:
        x -= eoTxtLineWidth(font,txt,len)/2.0;
      break;
      case TXT_RIGHT:
        x -= eoTxtLineWidth(font,txt,len);
      break;
      default:
        eoPrint("eoTxtWrite: No valid positioning (%i) specified, valid values are defined in eoTxtWrite.h", pos);
        break;
    }
  }

  glPushMatrix();
  glLoadIdentity();
  glTranslatef( x,y,0 );

  if(!ignoreColors)
    glColor4fv( txtColors[1] ); //Index 1 == White

  glEnable( GL_TEXTURE_2D );
  glBindTexture( GL_TEXTURE_2D, fonts[font].texture );
  for(i=0; i<len;i++)
  {
    int color = _controlPair( txt,len,i );
    if( color > -1 )
    {

      if(!ignoreColors)
        glColor4fv( txtColors[color] );
      i+=2;

    } else if( txt[i] == '\n' )
    {
      i++;
      lines++;
      glLoadIdentity();
      glTranslatef( x,y+(fonts[font].height*lines),0 );
    }
    glCallList( txt[i]+fonts[font].dlBase );
  }
  glPopMatrix();
}

void eoTxtWrite(int_fast8_t font, int_fast8_t pos, const char* txt, GLfloat x, GLfloat y)
{
  _gltxt(FALSE, font,pos,txt,x,y);
}


void eoTxtWriteShadow(int_fast8_t font, int_fast8_t pos, const char* txt, GLfloat x, GLfloat y)
{
  glColor4f(0,0,0,1);
  _gltxt(TRUE, font,pos,txt,x+1,y+1);
  _gltxt(FALSE, font,pos,txt,x,y);
}


GLfloat eoTxtHeight( int_fast8_t font ) { return( fonts[font].height ); }
