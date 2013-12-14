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

#include "strings.h"
#include <stdlib.h>
#include <string.h>

void stripNewLine(char* str)
{
  int i;
  for(i=0; i<strlen(str); i++)
  {
    if( str[i] == '\r' || str[i] == '\n' )
      str[i]='\0';
  }
}

void stripTabs(char* str)
{
  int i,ii=0;
  int len = strlen(str);
  char* dest = malloc( len+1 );
  memset(dest, 0, len+1 );
  for(i=0; i<len; i++)
  {
    if( str[i] != 0x9 )
    {
      dest[ii]=str[i];
      ii++;
    }
  }

  printf("Before: %s\n", str);
  strcpy(str,dest);
  printf("After: %s\n", str);
  free(dest);

}


int splitVals(char ch,const char* buf, char* set, char* val)
{
  int strpos=0;
  set[0]='\0';
  val[0]='\0';
  if(buf[0]!='#')
  {
    for(strpos=0; strpos<strlen(buf);strpos++)
    {
      //Here's a setting!
      if(buf[strpos]==ch && strpos > 0)
      {
        strcpy(set,buf);
        set[strpos]='\0';
        strcpy(val,buf+strpos+1);
        return(1);
      }
    }
  }

  //Simply fill the line
  strcpy(set,buf);


  return(0);
}

char** explode(char delim, char* buf, int size)
{
  char** t = malloc(sizeof(char*)*size);
  int pos;
  int start=0;
  int numFound=0;

  int lPos=0;

  //Clear all pointers so those can be used to check if they point to a string.
  for(pos=0; pos < size; pos++)
    t[pos]=0;

  for(pos=0; pos<strlen(buf); pos++)
  {
    if(buf[pos]==delim)
    {
      lPos=pos;
      t[numFound] = malloc(sizeof(char)*(pos-start+2));
      strncpy( t[numFound], buf+start, pos-start );
      t[numFound][pos-start]=0; //0 terminate

      start=pos+1;
      numFound++;
    }
  }

  if(lPos!=pos && numFound < size)
  {
    t[numFound] = malloc(sizeof(char)*(pos-start+2));
    strncpy( t[numFound], buf+start, pos-start );
    t[numFound][pos-start]=0; //0 terminate
  }

  return(t);
}

//Return position of last instance of c
//Return 0 if none found
int charrpos(const char* str, char c)
{
  int i;
  for(i=strlen(str); i != 0; i--)
  {
    if(str[i]==c)
    {
      return(i);
    }
  }
  return(0);
}
