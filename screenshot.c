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

#include "types.h"
#include "screenshot.h"
#include "gfxeng.h"
#include "console.h"
void eoGfxScreenshot(const char* name)
{
  FILE *fscreen;

  char cName[256];
  int i = 0;
  bool found=0;
  if( !name )
  {
    while(!found)
    {
      sprintf(cName, "./screenshot_%i.tga",i);
      fscreen = fopen(cName,"rb");
      if(fscreen==NULL)
        found=1;
      else
        fclose(fscreen);
        i++;
    }
  } else {
    strcpy(cName,name);
  }

  int nS = eoSetting()->res.x * eoSetting()->res.y * 3;
  GLubyte *px = malloc( sizeof(GLubyte)*nS);
  if(!px)
  {
    eoPrint("Screenshot: %s failed, couldn't allocate %i bytes.", cName, nS);
    return;
  }

  fscreen = fopen(cName,"wb");
  if(!fscreen)
  {
    eoPrint("Screenshot: %s failed, couldn't open file for binary writing.",cName);
    return;
  }

  glPixelStorei(GL_PACK_ALIGNMENT,1);
  glReadPixels(0, 0, eoSetting()->res.x, eoSetting()->res.y, GL_BGR, GL_UNSIGNED_BYTE, px);

  unsigned char TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};
  unsigned char header[6] = {eoSetting()->res.x%256,eoSetting()->res.x/256,eoSetting()->res.y%256,eoSetting()->res.y/256,24,0};
  fwrite(TGAheader, sizeof(unsigned char), 12, fscreen);
  fwrite(header, sizeof(unsigned char), 6, fscreen);

  fwrite(px, sizeof(GLubyte), nS, fscreen);
  fclose(fscreen);
  free(px);
  eoPrint("Screnshot saved: %s", cName);
}

int screenShotConsole( const char* name, void* data )
{
  eoGfxScreenshot(name);
  return( CON_CALLBACK_HIDE_RETURN_VALUE );
}

void screenShotInput( inputEvent* e )
{
  eoGfxScreenshot(NULL);
}
