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

#include "camera.h"
#include <stdio.h>
#include "gfxeng.h"
#include <math.h>
#include "console.h"
#include "types.h"


typedef struct {
  int8_t version;    //File version
  int8_t floatSize;  //Floatsize in bytes
  int8_t endian;     //Little or big endian
  int32_t frames;    //Number of frames in file
} camFileHeader;



//Camera data
static camData cam;

static bool camFree;    //If freelook is enabled, eoCamPosSet camSetLook are ignored.
static int camPlaybackState;
static int camPlaybackFrame;
static camData* camPlaybackData;
static FILE* camFile;
static listItem* camRecordData;
static camFileHeader camHead;
static int camPlaybackPosition;
static void (*camFinishedCallback)();
static GLfloat camMoveSpeed;
static GLfloat camMouseSens;

static int camGrab;
static int camLockLook;

vec3 relStartPos;
vec3 relStartTar;

int eoCamGetPlaybackState()
{
  return( camPlaybackState );
}

int cameraBeginRecord( const char* args, void* data )
{
  //Only when stopped
  if( camPlaybackState != CAM_PLAYSTATE_STOPPED )
  {
    eoPrint("Can't start recording while playing or recording.");
    return( CON_CALLBACK_HIDE_RETURN_VALUE );
  }

  //Open the file
  camFile = fopen( args, "wb" );

  //Is it open?
  if( camFile )
  {
    //Set data we need

    camPlaybackFrame = 0;
    camPlaybackData = 0;
    camRecordData = initList();
    eoExec( "camfree 1");
    camPlaybackPosition = CAM_PLAYBACK_POSITION_ABSOLUTE;
   // eoPrint( TXTCOL_YELLOW"Use camrecrel 1 to set relative recording.");
    eoPrint(TXTCOL_YELLOW"Press SPACE to start recording to %s. Space again to stop", args);
    eoExec( "bind space camRecordStart" );
//    _consoleToggle( NULL );
  } else {
    eoPrint("Can't open '%s' for writing.", args);
  }


  return( CON_CALLBACK_HIDE_RETURN_VALUE );
}

int cameraRecordRelative( const char* args, void* data )
{
  int i = atoi(args);
  if( i )
  {
    eoPrint("Relative recording enabled.");
    camPlaybackPosition = CAM_PLAYBACK_POSITION_RELATIVE;
  } else {
    eoPrint("Relative recording disabled.");
    camPlaybackPosition = CAM_PLAYBACK_POSITION_ABSOLUTE;
  }
  return( CON_CALLBACK_HIDE_RETURN_VALUE );
}

void camRecordStart(inputEvent* e)
{
  if( camFile )
  {
    eoExec( "bind space camRecordStop" );
    eoPrint("Recording... Press space to stop!");
    camPlaybackState = CAM_PLAYSTATE_RECORDING;

    relStartPos = cam.pos;
    relStartTar = cam.target;

  }
}

void camRecordStop( inputEvent* e)
{
  if( camFile &&  camPlaybackState == CAM_PLAYSTATE_RECORDING )
  {
     eoPrint("Recording stopped.");

     struct list* it = camRecordData;
     camData* dat;
     int f=0;
     //Setup file header
     camFileHeader header;
     header.version = 1;
     header.endian = 1;
     header.floatSize = sizeof(GLfloat);
     header.frames = listSize( camRecordData );

     //Convert list to array
     camPlaybackData = malloc( sizeof(camData)*header.frames );
     while( (it=it->next) )
     {
       dat = (camData*)it->data;
       camPlaybackData[f] = *dat;
       free(dat);
       f++;
     }

     //Write data to file
     //Header first
     fwrite( &header, sizeof(camFileHeader), 1, camFile );

     //Data next
     fwrite( camPlaybackData, sizeof(camData), header.frames, camFile );
     //Close file, reset data
     fclose( camFile );
     camFile=NULL;
     free( camPlaybackData );
     camPlaybackData=0;

     //Free list
     freeList(camRecordData);
     camPlaybackState=CAM_PLAYSTATE_STOPPED;
  } else {
    eoPrint("Not recording...");
  }
}

//Play movements from fileName, if finishedCallback is defined, it will be called when the recording stops.
void eoCamRecPlay( const char* fileName, int absolute, void (*finishedCallback)() )
{
  eoPrint("Playing %s (cb: %p)..", fileName, finishedCallback );
  //Only if stopped
  if( camPlaybackState != CAM_PLAYSTATE_STOPPED )
  {
    eoPrint("Camera must be stopped before playing a recording");
  }

  //Disable input
  eoExec( "camfree 0" );
  //Reset data
  camPlaybackFrame=0;

  //Open the file
  camFile = fopen( fileName, "rb" );

  //Open?
  if( camFile )
  {
    //Read header
    int nr =fread( &camHead, sizeof( camFileHeader ), 1, camFile );
    if( nr != 1 )
    {
      eoPrint("Couldn't read header from camera file '%s' correctly.", fileName);
    }
    //Check header
    if( camHead.version != 1 )
      eoPrint("Camera file '%s' wrong version", fileName);

    //Allocate space
    camPlaybackData = malloc( sizeof(camData)*camHead.frames );

    //copy data
    nr = fread( camPlaybackData, sizeof(camData), camHead.frames, camFile );
    if( nr != camHead.frames )
    {
      eoPrint("Error: Read %i frames of the %i specified in header of '%s'", fileName );
    }

    camPlaybackState = CAM_PLAYSTATE_PLAYING;
    camPlaybackPosition = absolute;

    camFinishedCallback = finishedCallback;

    //close file
    fclose(camFile);
  } else {
    eoPrint("Couldn't open %s", fileName);
  }
}

int camConPlayRec( const char* args, void* data )
{
  char buf[1024];
  eoPrint("Playing recording: %s", args);
  strcpy(buf, args);
  eoCamRecPlay( buf, CAM_PLAYBACK_POSITION_ABSOLUTE, NULL );
  _consoleToggle( NULL );
  return( CON_CALLBACK_HIDE_RETURN_VALUE );
}


void eoCamRecStop()
{
  if( camPlaybackState == CAM_PLAYSTATE_PLAYING )
  {
    camPlaybackState=CAM_PLAYSTATE_STOPPED;
    free( camPlaybackData );
  }
}

void _camPlayback()
{
  if(camPlaybackPosition == CAM_PLAYBACK_POSITION_ABSOLUTE)
  {
    cam = camPlaybackData[camPlaybackFrame];
  } else {
    //Broken
  }

  camPlaybackFrame++;

  if( camPlaybackFrame == camHead.frames )
  {
    //Clean up, we stopped playing
    eoCamRecStop();
    //Call finished callback
    if( camFinishedCallback )
      (*camFinishedCallback)();
  }
}

void _camRecord()
{
  //add frame to list
  camData* d = malloc( sizeof(camData) );
  *d = cam;


  if( camPlaybackPosition == CAM_PLAYBACK_POSITION_RELATIVE )
  {
//    d->pos = eoVec3FromPoints( relStartPos,d->pos );
//   d->target = eoVec3FromPoints( relStartTar, d->target );
  }

  listAddData( camRecordData, (void*)d );

}

void camSetMatrix()
{

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum( -eoSetting()->aspect*cam.zoom, eoSetting()->aspect*cam.zoom, -1.0*cam.zoom, 1.0*cam.zoom, cam.zNear, 5000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  gluLookAt(cam.pos.x, cam.pos.y, cam.pos.z, cam.target.x, cam.target.y, cam.target.z, 0,1,0);
}

void camBegin()
{
  //Playback or record movie?
  if( camPlaybackState != CAM_PLAYSTATE_STOPPED )
  {
    switch( camPlaybackState )
    {
      case CAM_PLAYSTATE_PLAYING:
        _camPlayback();
      break;
      case CAM_PLAYSTATE_RECORDING:
        _camRecord();
      break;
    }
  }

  camSetMatrix();

}


void eoCamPosSet(vec3 p)
{
  if( !camFree )
    cam.pos=p;
}

void eoCamTargetSet(vec3 p)
{
  if( !camFree )
    cam.target=p;
}

void eoCamSetZnear( GLfloat z )
{
  cam.zNear=z;
}

camData* camGet()
{
  return(&cam);
}

vec3 eoCamPosGet()
{
  return( cam.pos );
}

vec3 eoCamTargetGet()
{
  return( cam.target );
}

GLfloat eoCamZoomGet()
{
  return(cam.zoom);
}

static GLfloat xrot=0,yrot=0;
vec3 eoCamDirectionGet()
{
  return( eoVec3FromAngle( xrot, yrot ) );
}

void _camInput(inputEvent* e)
{
  if(e->key)
  {
    if( e->key->sym.sym == SDLK_w )
    {
      eoCamMoveForward(  camMoveSpeed );
    } else if( e->key->sym.sym == SDLK_s )
    {
      eoCamMoveBackward(  camMoveSpeed );
    } else if( e->key->sym.sym == SDLK_a )
    {
      eoCamMoveLeft(  camMoveSpeed );
    } else if( e->key->sym.sym == SDLK_d )
    {
      eoCamMoveRight(  camMoveSpeed );
    } else if( e->key->sym.sym == SDLK_q )
    {
      eoCamMoveUp(  camMoveSpeed );
    } else if( e->key->sym.sym == SDLK_e )
    {
      eoCamMoveDown(  camMoveSpeed );
    }
  } else if( e->mouse )
  {
    if( e->mouse->type == INPUT_EVENT_TYPE_MOTION && !camLockLook )
    {
      xrot += e->mouse->motion.xrel/camMouseSens;
      yrot += e->mouse->motion.yrel/camMouseSens;

      if( yrot < 0.0001 ) yrot = 0.0001;
      if( yrot > 3.1415925 ) yrot = 3.1415925;

      cam.target = eoVec3Add( cam.pos, eoVec3Scale( eoCamDirectionGet(), 25 ) );

    } else if( e->mouse->type == INPUT_EVENT_TYPE_BUTTON )
    {
      if( e->mouse->button.button == SDL_BUTTON_WHEELDOWN )
      {
        camMoveSpeed -= 0.01;
        if( camMoveSpeed < 0 ) camMoveSpeed = 0;
      } else if( e->mouse->button.button == SDL_BUTTON_WHEELUP )
      {
        camMoveSpeed += 0.01;
        if( camMoveSpeed > 3 ) camMoveSpeed = 3;
      }
    }
  }
}

int cameraFreeLook( const char* args, void* data )
{
  if( args && atoi(args)==1 )
  {
    eoPrint("Camera freelook ^3enabled^1.");
    //Set input rotations somewhere near
    ///TODO: Set xrotation correct so mouse does not "jump" first time
    vec2 p = engRadFromPoints( cam.pos, cam.target );
    xrot = p.x;
    yrot = p.y;

    //Hook to input events.
    eoInpAddHook( INPUT_EVENT_ALL_KEYS, INPUT_FLAG_HOLD,0, _camInput );
    eoInpAddHook( INPUT_EVENT_MOUSE, INPUT_FLAG_MOVEMENT|INPUT_FLAG_UP,0, _camInput );
    camFree=TRUE;
    if( camGrab )
      SDL_WM_GrabInput( SDL_GRAB_ON );
  } else {
    eoPrint("Camera freelook ^2disabled^1.");
    //Remove hook
    eoInpRemHook( INPUT_EVENT_ALL_KEYS, 0, _camInput );
    eoInpRemHook( INPUT_EVENT_MOUSE,0, _camInput );
    camFree=FALSE;
    SDL_WM_GrabInput( SDL_GRAB_OFF );
  }
  return( CON_CALLBACK_HIDE_RETURN_VALUE );
}

int cameraSetSens( const char* args, void* data )
{
  if( !args )
  {
    eoPrint("Current sens: %f", camMouseSens);
    return( CON_CALLBACK_HIDE_RETURN_VALUE );
  }

  GLfloat f = atof(args);
  if( f > 49 && f < 1001 )
  {
    camMouseSens = f;
    eoPrint("^3 Camera sensitivity set to '^2%f^3'.",f);
  } else {
    eoPrint("Valid sensitivity is between 50 and 1000 where 1000 is slowest.");
  }
  return( CON_CALLBACK_HIDE_RETURN_VALUE );
}

int cameraGrabCursor( const char* args, void* data)
{
  if( args && atoi(args)==1 )
  {
    camGrab=1;
  } else {
    camGrab=0;
  }
  return( camGrab );
}

void setCameraLockLook(int l)
{
    camLockLook=l;
}

int cameraLockLook( const char* args, void* data)
{
  if( args && atoi(args)==1 )
  {
    camLockLook=1;
  } else {
    camLockLook=0;
  }
  return( camLockLook );
}


void eoCamZoomSet( GLfloat zoom )
{
  cam.zoom = zoom;
}

void eoCamMoveForward( GLfloat l )
{
  vec3 dir;
  dir = eoVec3FromPoints(cam.pos, cam.target);
  dir = eoVec3Normalize( dir );
  dir = eoVec3Scale( dir, l );

  cam.pos = eoVec3Add( dir, cam.pos );
  if(!camLockLook)
    cam.target = eoVec3Add( dir, cam.target );
}

void eoCamMoveBackward( GLfloat l )
{
  vec3 dir;
  dir = eoVec3FromPoints(cam.pos, cam.target);
  dir = eoVec3Normalize( dir );
  dir = eoVec3Scale( dir, l );
  cam.pos.x -= dir.x;
  cam.pos.y -= dir.y;
  cam.pos.z -= dir.z;

  if(!camLockLook)
  {
    cam.target.x -= dir.x;
    cam.target.y -= dir.y;
    cam.target.z -= dir.z;
  }
}

void eoCamMoveLeft( GLfloat l )
{
  vec3 dir;
  dir = eoVec3FromPoints(cam.pos, cam.target);
  dir = eoVec3Normalize( dir );
  GLfloat a = atan2( dir.z, dir.x );

  GLfloat c = cos( a+4.71238898 )*l;
  GLfloat s = sin( a+4.71238898 )*l;
  cam.pos.x += c;
  cam.pos.z += s;

  if(!camLockLook)
  {
    cam.target.x += c;
    cam.target.z += s;
  }
}

void eoCamMoveRight( GLfloat l )
{
  vec3 dir;
  dir = eoVec3FromPoints(cam.pos, cam.target);
  dir = eoVec3Normalize( dir );
  GLfloat a = atan2( dir.z, dir.x );

  GLfloat c = cos( a+4.71238898 )*l;
  GLfloat s = sin( a+4.71238898 )*l;
  cam.pos.x -= c;
  cam.pos.z -= s;
  if(!camLockLook)
  {
    cam.target.x -= c;
    cam.target.z -= s;
  }
}

void eoCamMoveUp( GLfloat l )
{
  cam.pos.y += l;

  if(!camLockLook)
    cam.target.y += l;
}

void eoCamMoveDown( GLfloat l )
{
  cam.pos.y -= l;

  if(!camLockLook)
    cam.target.y -= l;
}


void camInit()
{
  memset( &cam, 0, sizeof(camData) );
  cam.zoom = 1.0;
  cam.zNear=1.5; //Not very scientific..
  camFree=0;
  camPlaybackFrame=0;
  camPlaybackState=0;
  camPlaybackData=0;
  camRecordData=0;
  camMoveSpeed = 0.15;
  camMouseSens = 300;
  camGrab=1;
  camLockLook=0;

  eoInpAddFunc( "camRecordStart", "Starts recording file.", camRecordStart, INPUT_FLAG_UP );
  eoInpAddFunc( "camRecordStop", "Stops recording file.", camRecordStop, INPUT_FLAG_DOWN );
}
