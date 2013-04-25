///This file is to be deleted and is only a todo of functions that need to be refactored
#ifdef NOT_DEFINED_EVER
todo:
Alt i main skal v��k og kaldes af eoInit

  //Console functions
  eoPrint //Print in console
  eoExec  //Execute console command
  eoVarAdd //Add variable to console
  eoFuncAdd //Add function to console

  //Math utillities
  eoBestPOT //Get the smallest number that is equal to, or larger than input.
  eoRandFloat //Get a random floating point number
  eoVec3FromPoints // Unnormalized vector pointing from A towards B (This subtracts two vectors)
  eoVec3Len //
  eoVec3Normalize
  eoVec3Scale
  oVec3FromAngle; //
  eoVec3Add

  //Game world functions
  eoObjAdd //Add a baked object to game world
  eoObjAttach //Attach a baked object to another backed object
  eoObjBake;   //eoObjBake
  eoObjCreate
  eoWorldClear  //Clear the world
  eoObjDel //Delete object from world
  eoPauseGet //
  eoPauseSet //

  //Particle emitter functions
  eoPsysBake
  eoPsysEmit
  eoPsysFree
  eoPsysNewEmitter

  //3D model funtions
  eoModelFree;  //eoModelFree
  eoModelLoad;  //eoModelLoad

  //Sprite functions
  eoSpriteNew;
  eoSpriteDel;
  eoSpriteBaseDel
  eoSpriteBaseLoad
  eoSpriteScale;

  //Sound functions
  eoMusicFadeTo
  eoMusicFree
  eoSampleFree
  eoMusicLoad
  eoSampleLoad
  eoSamplePlay
  eoMusicStop

  //Time functions
  eoTicks
  eoTicksReset

  //Input functions
  eoInpAddHook
  eoInpRemHook

  //GUI functions
  eoGuiContextCreate
  eoGuiContextGet
  eoGuiContextSet
  eoGuiContextDelete
  eoGuiAddWindow
  eoGuiWinBgCol
  eoGuiWinBorCol
  eoGuiBtnSetTxt
  eoGuiAddLabel
  eoGuiAddButton
  eoGuiAddImage
  eoGuiAddTextBox
  eoGuiShowCursor
  eoGuiShow
  eoGuiHide
  eoGuiIsActive
  eoGuiFade

  //Text functions
  eoTxtHeight
  eoTxtLineWidth
  eoTxtWrite
  eoTxtWriteShardow

  //Camera functions
  eoCamTargetGet
  eoCamPosGet
  eoCamZoomGet
  eoCamTargetSet
  eoCamPosSet
  eoCamZoomSet
  eoCamRecPlay
  eoCamRecStop
  eoCamMoveForward
  eoCamMoveBackward
  eoCamMoveLeft
  oCamMoveRight
  eoCamMoveUp
  eoCamMoveDown


  //Graphics engine functions
  eoGfxBillboardBegin
  eoGfxBillBoardEnd
  eoGfxFboCreate
  eoGfxFboRenderBegin
  eoGfxFboRenderEnd
  eoGfxFboDestroy
  eoGfxLoadTex
  eoGfxTexFromSdlSurface
  eoGfxScreenshot

  //Settings function
  eoSetting
#endif


#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "gl.h"
#include "eng.h"
#include "gfxeng.h"
#include "console.h"
#include "vboload.h"
#include "tick.h"
#include "sound.h"
#include "camera.h"
#include "sprite.h"
#include "version.h"
#include "particles.h"
#include "game.h"
#include "input.h"
#include "gui.h"

int engVarDone;
int engVarShowTimes;
SDL_Surface* engVarScreen;

int eoInitAll(int argc, char** argv, const char* datadir)
{
  #ifdef WIN32
  FILE *stream;
  stream = freopen("CON", "w", stdout);
  #endif

  DataSetDir( datadir );

  //Init SDL
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
  atexit(SDL_Quit);

  //Set native resolution and fullscreen if nobody tells us different
  eoSetting()->res.x = SDL_GetVideoInfo()->current_w;
  eoSetting()->res.y = SDL_GetVideoInfo()->current_h;

  eoSetting()->fullScreen=SDL_FULLSCREEN;

  //If we're going windowed mode, we go windowed mode except if told otherwise.
  if( argc > 2 )
  {
    eoSetting()->res.x = atoi(argv[1]);
    eoSetting()->res.y = atoi(argv[2]);
    eoSetting()->fullScreen = 0;
    if( argc == 4 )
    {
      eoSetting()->fullScreen = SDL_FULLSCREEN;
    }
  }

    eoSetting()->aspect = (float)eoSetting()->res.x/(float)eoSetting()->res.y;



    SDL_Surface* engVarScreen = SDL_SetVideoMode(eoSetting()->res.x, eoSetting()->res.y, 32, SDL_OPENGL | eoSetting()->fullScreen );
    if ( ! engVarScreen ) {
      eoPrint("Couldn't set videomode: %s\n", SDL_GetError());
      exit(2);
    }

    SDL_WM_SetCaption("Excessive Overkill", "Excessive Overkill");
    SDL_ShowCursor( SDL_DISABLE );
   /// <--- InitSDL

    sndInit();
    eoPrint("Sound initialized.");

    inputInit();
    eoPrint("Input initialized.");

    gfxEngInit();
    eoPrint("Gfx engine initialized.");

    guiInit();
    eoPrint("GUI initialized.");

    camInit();
    eoPrint("Camera initialized.");

    eoTicksReset();
    eoPrint("Timekeeping initialized.");

    psysInit();
    eoPrint("Particle systems initialized.");

    eoGameInit();
    eoPrint("Game world initialized.");

    engVarDone=0;
    engVarShowTimes=0;

    eoVarAdd(CON_TYPE_INT,0, &engVarShowTimes, "showtimes");
    eoVarAdd(CON_TYPE_INT,0, &engVarDone, "quit");

    return(1);
  }


void eoMainLoop()
{

  while ( ! engVarDone )
  {
    tickStartFrame(); //Keep track of time
    SDL_Event event;

    while ( SDL_PollEvent(&event) )
    {
      switch(event.type)
      {
        case SDL_QUIT:
          engVarDone = 1;
        break;
        case SDL_KEYDOWN:
          inputKeyDown( event.key.keysym );
        break;
        case SDL_KEYUP:
          inputKeyUp( event.key.keysym );
        break;
        case SDL_MOUSEMOTION:
          inputMouseMove( event.motion );
        break;
        case SDL_MOUSEBUTTONDOWN:
          inputMouseButton( event.button );
        break;
        case SDL_MOUSEBUTTONUP:
          inputMouseButton( event.button );
        break;
        case SDL_JOYAXISMOTION:
          inputJoyMove( event.jaxis );
        break;
        case SDL_JOYBUTTONDOWN:
          inputJoyButton( event.jbutton );
        break;
        case SDL_JOYBUTTONUP:
          inputJoyButton( event.jbutton );
        break;
      }
    }

  //Run keys
  inputRunKeys();

  engRender();

  gameRun();

  //Render 2D overlays.
  guiDraw();

  //Music
  sndRun();

  tickStopLogic();
  SDL_GL_SwapBuffers();

  //If we don't have vsync on, or if we are above 60 fps.
  int dt = ticksLogic();
  if( !(dt>0 && dt < 16) ) dt=0;
  #ifndef WIN32
    usleep(16666-dt*1000);
  #else
    Sleep(16-dt);
  #endif
  if( engVarShowTimes )
    eoPrint("Frame time: %i ms. Code time: %i ms.", eoTicks(), dt );
  }
}
