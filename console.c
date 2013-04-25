/******************************************************************************
 * This file is part of ExcessiveOverkill.                                    *
 *                                                                            *
 * Copyright 2011 Jimmy B��gh Christensen                                      *
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

#define _GNU_SOURCE
#include <stdio.h>

#include "console.h"


#ifdef WIN32
    #include "3rd/winvasp.h"
#endif

#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "types.h"
#include "gltxt.h"
#include "gfxeng.h"
#include "input.h"
#include "data.h"
#include "strings.h"
#define CONSOLE_LINES 40
#define CON_MAX_CMD_LEN 511

static char buf[1024];
static char cmd[512];
static char arg[512];
static char val[512];
static char command[512];

static GLfloat consoleHeight;
static listItem* con;
static listItem* cVs;
static listItem* history;
static int editPos=0;
static int histPos=0;
static bool init=0;
static GLuint consoleList;
static int curLine=0;
static bool showConsole=0;
static GLfloat conBgColor[4] = { 1.0, 1.0, 1.0, 0.95 };

typedef struct {
  int type;
  int len;
  int (*callback)(const char*, void* data);
  void* data;
  char* name;
} cVar;

void consoleAutoComplete()
{
  listItem* itCv; //List of cVars
  cVar* c;
  itCv = cVs;
  char* cvName;
  char* match=0;
  int cmdLen = strlen( command );
  int i=0;
  int len=0;
  int lenBest=0;
  int numMatches=0;

  //For each registered cvar
  while( (itCv=itCv->next) )
  {
    c = (cVar*)itCv->data;
    cvName = c->name;

    //If the cvar is longer than the command being entered
    if( strlen( cvName ) >= cmdLen )
    {
      len=0;
      //Loop through the characters in the command, comparing them to cvName
      for( i=0; i < cmdLen; i++ )
      {

          if( cvName[i] == command[i] )
          {
            len++;
          } else {
            len=0;
            break;
          }

      }
      if( len )
      {
        eoPrint("Match: %s", cvName );
        numMatches++;
      }
    }

    if( len>lenBest )
    {
      lenBest=len;
      match=cvName;
    }
  }

  //We only autocomplete when there is a single match.
  if( numMatches == 1 )
  {
    sprintf(buf, "%s ", match );
    strcpy( command, buf );
    editPos = strlen( command );
  } else if( numMatches > 1)
  {
    eoPrint(" -- %i possible --", numMatches );
  } else if( numMatches == 0 )
  {
    eoPrint("Nothing starting with '%s' found.", command);
  }

}

void _consoleInput(inputEvent* e)
{
  int c = e->key->sym.sym;
  int i;
  if(c != SDLK_RETURN)
  {
    if( c== SDLK_BACKSPACE )
    {
      if(editPos>0)
      {
        editPos--;
        command[editPos]=0;
      }
    } if( e->key->sym.sym == SDLK_DELETE )
    {
      if(editPos < strlen(command) && strlen(command) > 0)
      {
        //Remove character at editPos from string
        for( i=editPos; i < strlen(command)-1;i++)
        {
          command[i]= command[i+1];
        }
      }
    } else if( e->key->sym.sym==SDLK_LEFT )
    {
      if( editPos>0 ) editPos--;
    } else if( e->key->sym.sym==SDLK_RIGHT )
    {
      if( editPos < strlen( command ) ) editPos++;
    } else if( e->key->sym.sym == SDLK_UP )
    {
      if(histPos<listSize(history))
      {
        histPos++;
        strcpy( command, (char*)listGetItemData( history,histPos-1 ) );
        editPos = strlen( command );
      }
    } else if( e->key->sym.sym == SDLK_DOWN)
    {
      if(histPos>0)
      {
        histPos--;
        strcpy( command, (char*)listGetItemData( history,histPos ) );
        editPos = strlen( command );
      }

    } else if( e->key->sym.sym == SDLK_TAB )
    {
      if( strlen( command ) > 0 )
      {
        consoleAutoComplete();
      }
    } else {
      if( (e->key->sym.sym > 24 && e->key->sym.sym < 255) || e->key->sym.sym ==267 )
      {
        if( e->key->sym.sym == 267 )
          c = 47;
        if(e->key->sym.mod&KMOD_SHIFT)
        {
          if( e->key->sym.sym == SDLK_7 )
            c = 47;
          else
          c=toupper(c);
        }
        if( editPos < CON_MAX_CMD_LEN )
        {
          command[editPos] = c;
          editPos++;
          command[editPos] = 0;
        }
      }
    }
  } else {
    if( strlen(command) )
    {
      editPos=0;
      histPos=0;
      char* he = malloc( (strlen(command)+1)*sizeof(char) );
      strcpy( he, command );
      listInsertData( history, (void*)he, 0);

      eoExec(command);
      command[0] = 0;
    }
  }
}

void _consoleToggle(inputEvent* e)
{
  showConsole=!showConsole;
  if(showConsole)
  {
    //If get's shown, we subscribe to all key events
    eoInpAddHook( INPUT_EVENT_ALL_KEYS, INPUT_FLAG_DOWN|INPUT_FLAG_EXCLUSIVE,0,&_consoleInput );
  } else {
    //If hides, unsubscribe to key events
    eoInpRemHook( INPUT_EVENT_ALL_KEYS, 0, &_consoleInput );
  }

}

int _consoleHelp( const char* arg, void* data )
{
  eoPrint("Commands:");
  eoPrint(" ^2help   ^3You're reading it.");
  eoPrint(" ^2quit 1 ^3Exit program. ");
  eoPrint(" ^2cvars  ^3Show bound cvars/cfunctions");
  return( CON_CALLBACK_HIDE_RETURN_VALUE );
}

int _consoleVars( const char* arg, void* data )
{
  listItem* it=cVs;
  eoPrint("Vars/Functions hooked into console:");
  while( (it=it->next) )
  {
    cVar* cv = (cVar*)it->data;
    eoPrint("Name: '^3%s^1'", cv->name);
  }
  return( CON_CALLBACK_HIDE_RETURN_VALUE );
}

int _conEcho( const char* arg, void* unused )
{
  eoPrint("%s", arg);
  return(CON_CALLBACK_HIDE_RETURN_VALUE);
}

int _conExecScript( const char* arg, void* unused )
{
  FILE* scr = fopen( arg, "r" );
  size_t bufSize = 1023;
  char* buf = malloc(bufSize+1);

  if( !scr )
  {
    eoPrint("exec: Could not open '%s'.", arg);
  } else {
    eoPrint("exec: Running commands from '%s' ...", arg);
    while(fgets( buf, bufSize, scr) )
    {
      stripNewLine(buf);
      if( buf[0] != '#' && buf[0] != ' ' && buf[0] != 0 )
      {
        eoExec( buf );
      }
    }
    fclose(scr);
  }

  free(buf);
  return(CON_CALLBACK_HIDE_RETURN_VALUE);
}

void consoleInit()
{
  consoleHeight = eoTxtHeight(FONT_SYS) * (CONSOLE_LINES+1);

  consoleList = glGenLists(1);
  glNewList(consoleList, GL_COMPILE);
    glLoadIdentity();
    glTranslatef(0, -eoSetting()->res.x+consoleHeight, 0 );

    glBindTexture(GL_TEXTURE_2D, eoGfxLoadTex( Data( "/data/gfx/", "conback.png" ) ) );
    glBegin(GL_QUADS);
      glTexCoord2f(0,1); glVertex2f(0,eoSetting()->res.x);
      glTexCoord2f(1,1); glVertex2f(eoSetting()->res.x, eoSetting()->res.x);
      glTexCoord2f(1,0); glVertex2f(eoSetting()->res.x,0);
      glTexCoord2f(0,0); glVertex2f(0,0);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glColor4f( 0,0,0.2, 0.6);
    glBegin(GL_QUADS);
      glVertex2f(0,eoSetting()->res.x);
      glVertex2f(eoSetting()->res.x, eoSetting()->res.x);
      glVertex2f(eoSetting()->res.x,eoSetting()->res.x-13);
      glVertex2f(0,eoSetting()->res.x-13);
    glEnd();

    glBegin(GL_LINES);
    glColor4f( 1,0,0, 0.4);
    glVertex2f(0, eoSetting()->res.x);
    glVertex2f(eoSetting()->res.x, eoSetting()->res.x);
    glColor4f( 0,0,0, 0.6);
    glVertex2f(0, eoSetting()->res.x-13);
    glVertex2f(eoSetting()->res.x, eoSetting()->res.x-13);

    glEnd();

  glEndList();

  command[0]=0;

  //Hook console to F1 key.
  eoInpAddHook( INPUT_EVENT_KEY, INPUT_FLAG_DOWN|INPUT_FLAG_EXCLUSIVE, SDLK_F1, &_consoleToggle );

  //Hook the echo function.
  eoFuncAdd( _conEcho,NULL, "echo" );

  //Hook the exec function.
  eoFuncAdd( _conExecScript, NULL, "exec" );

  eoPrint("GL Console initialized.",CONSOLE_LINES,eoTxtHeight(FONT_SYS));
}

void eoPrint(const char* format, ...)
{
  char* buffer;
  int i;
  va_list args;
  va_start (args, format);
  if( vasprintf(&buffer,format,args) < 0 )
  {
    printf("Error: vasprintf failed in eoPrint.\n");
    return;
  }
  va_end (args);

  if(!init)
  {
    con = initList();
    cVs = initList();
    command[0] = 0;
    history = initList();
    eoVarAdd( CON_TYPE_FLOAT_ARRAY, 4, conBgColor, "concol" );
    eoFuncAdd( _consoleHelp, NULL, "help" );
    eoFuncAdd( _consoleVars, NULL, "cvars");
    init=1;
  }

  char* line = malloc( (strlen(buffer)+1+100)*sizeof(char) );
  sprintf(line, "^4%i^0>^1 %s", curLine, buffer );
  free(buffer);
  listAddData( con, (void*)line );

  //Output line to stdout too.
  for(i=0; i<strlen(line); i++)
  {
    if( line[i]=='^' && i+1 <= strlen(line) )
    {
      i+=1;
    } else {
      putchar( line[i] );
    }
  }
  putchar( '\n' );

  //Increment line number
  curLine++;
  //When we reach 500 lines, we cut off the first line.
  if(curLine>CONSOLE_LINES)
  {
    //Free line
    free( (char*)con->next->data );
    //Very fast removal of first object since lists are ordered.
    listRemoveItem( con, con->next );
  }

}

void consoleRender()
{
  int line=1;
  GLfloat ypos=0;
  listItem* it = con;
  glColor4fv( conBgColor );
  glCallList(consoleList);
  //Draw the text
  GLfloat height = eoTxtHeight(FONT_SYS);
  while( (it=it->next) )
  {
    char* str = (char*)it->data;
    eoTxtWriteShadow( FONT_SYS, TXT_LEFT,str, 5, ypos );
    ypos += height;
    line++;
  }


  eoTxtWriteShadow( FONT_SYS, TXT_LEFT, command, 5, consoleHeight-eoTxtHeight(FONT_SYS) );
  eoTxtWriteShadow( FONT_SYS, TXT_LEFT, "^3_", 5+(eoTxtLineWidth(FONT_SYS, command, editPos)) , consoleHeight-eoTxtHeight(FONT_SYS) );
}

void eoVarAdd( int type, int len, void* data, const char* name )
{
  cVar* c = malloc(sizeof(cVar));
  c->name = malloc( (strlen(name)+1)*sizeof(char) );
  strcpy( c->name, name );
  c->data = data;
  c->type = type;
  c->len = len;
  c->callback = NULL;
  listAddData( cVs, (void*)c);
}

void eoFuncAdd( int(*callback)(const char*, void*),void* data, const char* name )
{
  cVar* c = malloc(sizeof(cVar));
  c->name = malloc( (strlen(name)+1)*sizeof(char) );
  strcpy( c->name, name );
  c->data = data;
  c->type = CON_INTERNAL_TYPE_FUNC;
  c->len = 0;
  c->callback = callback;
  listAddData( cVs, (void*)c);
}


void cVarRem( const char* name )
{
  listItem* it = cVs;

  while( (it=it->next ) )
  {
    cVar* cv = (cVar*)it->data;
    if( strcmp( cv->name, name ) == 0 )
    {
      //Free Name (but not data since that's still in the program, and should be removed by allocator)
      free( cv->name );
      //Free svar
      free( cv );
      //Free listItem
      listRemoveItem( cVs, it );
      return;
    }
  }
}


void eoExec( const char* str )
{
  int i=0;
  //Split cmd into cmd part and argument part
  splitVals( ' ', str,cmd,arg  );
  bool found=FALSE;

  {itBegin( cVar*, cv, cVs )

    if( strcmp( cv->name, cmd ) == 0 )
    {
      found=TRUE;
      switch( cv->type )
      {
        case CON_TYPE_INT:
          if( strlen(arg) != 0 )
          {
            eoPrint("Integer '^2%s^1' changed from '^4%i^1' to '^3%s^1'", cmd, *((int*)cv->data), arg );
            *((int*)cv->data) = atoi( arg );
          } else {
            eoPrint("Integer '^2%s^1' is '^3%i^1'", cmd, *((int*)cv->data) );
          }
        break;
        case CON_TYPE_FLOAT:
          if( strlen(arg) != 0 )
          {
            eoPrint("Float '^2%s^1' changed from '^4%f^1' to '^3%s^1'", cmd, *((GLfloat*)cv->data),arg );
            *((GLfloat*)cv->data) = (GLfloat)atof( arg );
          } else {
            eoPrint("Float '^2%s^1' is '^3%f^1'", cmd, *((GLfloat*)cv->data) );
          }
        break;
        case CON_TYPE_STRING:
          if( strlen(arg) != 0 )
          {
            eoPrint("String '^2%s^1' changed from '^4%s^1' to '^3%s^1'", cmd, (char*)cv->data,arg );
            free( (char*)cv->data );
            cv->data = malloc( (strlen(arg)+1)*sizeof(char));
            strcpy( (char*)cv->data, arg );
          } else {
            eoPrint("String '^2%s^1' is '^3%s^1'", cmd, (char*)cv->data );
          }
        break;
        case CON_TYPE_FLOAT_ARRAY:
          if( strlen(arg) != 0 )
          {
            for(i=0;i<cv->len;i++)
            {
              splitVals(' ', arg, val, buf );
              eoPrint("^2%s^1[^2%i^1] = '^4%f^1' changed to '^3%f^1'", cmd, i, ((GLfloat*)cv->data)[i], (GLfloat)atof(val) );
              ((GLfloat*)cv->data)[i] = (GLfloat)atof(val);
              strcpy( arg, buf );
            }

          } else {
            for(i=0;i<cv->len;i++)
            {
              eoPrint("^3%s^1[^2%i^1] = '^3%f'", cmd, i, ((GLfloat*)cv->data)[i]);
            }
          }
        break;
        case CON_TYPE_VEC2:
          splitVals(' ', arg, val, buf );
          eoPrint("^3%s^1.^2x^1 = '^4%f' changed to '^3%f^1'", cmd, i, ((vec2*)cv->data)->x, (GLfloat)atof(val) );
          strcpy( arg, buf );
          eoPrint("^3%s^1.^2y^1 = '^4%f' changed to '^3%f^1'", cmd, i, ((vec2*)cv->data)->y, (GLfloat)atof(val) );
        break;
        case CON_TYPE_VEC3:
          if( strlen(arg) != 0 )
          {
            splitVals(' ', arg, val, buf );
            eoPrint("^3%s^1.^2x^1 = '^4%f' changed to '^3%f^1'", cmd, ((vec3*)cv->data)->x, (GLfloat)atof(val) );
            ((vec3*)cv->data)->x = (GLfloat)atof(val);
            strcpy( arg, buf );
            splitVals(' ', arg, val, buf );
            eoPrint("^3%s^1.^2y^1 = '^4%f' changed to '^3%f^1'", cmd, ((vec3*)cv->data)->y, (GLfloat)atof(val) );
            ((vec3*)cv->data)->y = (GLfloat)atof(val);
            strcpy( arg, buf );
            splitVals(' ', arg, val, buf );
            eoPrint("^3%s^1.^2z^1 = '^4%f' changed to '^3%f^1'", cmd, ((vec3*)cv->data)->z, (GLfloat)atof(val) );
            ((vec3*)cv->data)->z = (GLfloat)atof(val);
          } else {
            eoPrint("^3%s^1.^2x^1 = '^3%f^1'", cmd, ((vec3*)cv->data)->x);
            eoPrint("^3%s^1.^2y^1 = '^3%f^1'", cmd, ((vec3*)cv->data)->y);
            eoPrint("^3%s^1.^2z^1 = '^3%f^1'", cmd, ((vec3*)cv->data)->z);
          }
        break;
        case CON_INTERNAL_TYPE_FUNC:
          if( strlen(arg) != 0 )
           {
            i = (*cv->callback)(arg, cv->data);
          } else {
            i = (*cv->callback)(NULL, cv->data);
          }
          if( i != CON_CALLBACK_HIDE_RETURN_VALUE )
          {
            eoPrint( "^2%s^1(^3 %s^1 ) returned: '^2%i^1'", cmd,arg, i );
          }

        break;

      }
      return;
    }
  itEnd();}

  if( !found )
    eoPrint("^7cmd/cVar^2 %s^7 not found.", cmd);

}

bool consoleVisible()
{
  return( showConsole );
}
