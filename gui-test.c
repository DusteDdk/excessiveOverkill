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

#include "gui-test.h"
#include "console.h"
#include "gltxt.h"
#include "data.h"

#include "sound.h"
typedef struct {
  guiImage_s* img;
  guiButton_s* btn;
} animData;

typedef struct {
  int showing;
  guiWindow_s* oldContext; //The original context we override when showing this text
  guiWindow_s* contextA;
  guiWindow_s* contextB;
  animData anim;
  sound_s* sound;
  music_s* music;
} guiTest_s;

guiTest_s test;


void _btnGreatSuccess(void* data)
{
  guiWindow_s* btnMsg = eoGuiAddWindow(test.contextA, 0,0,400,200, "Thought bubble", BTN_SHOWCLOSE );

  eoGuiAddLabel(btnMsg, 0,0, "A human is a really big processor, with lots of\n"
                          "signal noise & lots of broken circuits, executing\n"
                          "a self modifying program til reads from every possible\n"
                          "input & computes based on these & on the sum of\n"
                          "all previous states. - Jimmy" );


}

void _testSetContext(void* data)
{
  guiWindow_s* win = (guiWindow_s*)data;
  eoGuiContextSet( win );
}

void _testAnimate( void* data )
{

  eoPrint("Here");
  if( test.anim.img->sprite->animPlaying )
  {
    eoGuiBtnSetTxt( test.anim.btn, "Stop!" );
  } else {
    eoGuiBtnSetTxt( test.anim.btn, "Start!" );
  }
  test.anim.img->sprite->animPlaying = !test.anim.img->sprite->animPlaying;
}

void _testSound( void* data )
{
  eoSamplePlay( test.sound, 128 );
}

void _testInit()
{
  test.showing=TRUE;
  //Remember what was there before.
  test.oldContext = eoGuiContextGet();

  //Create two contexts to show off some windows..
  test.contextA = eoGuiContextCreate();
  test.contextB = eoGuiContextCreate();

  guiWindow_s* mainWin = eoGuiAddWindow( test.contextA, 100,100,640,480, "A topwindow", BTN_SHOWCLOSE );
  eoPrint("Context.A=%p", test.contextA);
  eoGuiAddLabel( mainWin, 0,0, "To ^2make ^1a ^3pie^1, ^7one^1 must first ^4create^1 ^1the ^5universe^6!" )->font=FONT_LARGE;

  guiWindow_s* another = eoGuiAddWindow( test.contextA, 50,50, 500,265, "Another topwindow", BTN_SHOWCLOSE );

//  eoGuiAddImage( another, 0,0, Data("/data/gfx/", "bugs.spr") );

  eoGuiAddLabel( another, 0,211, "Excessive overkill is excessive\nIt'd be surprising if it's bugfree!" );


  guiWindow_s* anotherSub = eoGuiAddWindow(mainWin, 150,150, 400,300, "Window inside a window, MADNESS!", BTN_SHOWCLOSE );
  eoGuiWinBgCol( anotherSub, 0,1,0,0.2 );
  eoGuiWinBorCol( anotherSub, 1, 0, 0, 0.5 );
  anotherSub->showTitle=TRUE;
  eoGuiAddLabel(anotherSub, 0,0, "This one here windows ain't got no ^3buttons^1 on it!" );

  guiWindow_s* subsub = eoGuiAddWindow(anotherSub, 0,0,300,50, " ", BTN_HIDECLOSE );

  subsub->showTitle = FALSE;
  eoGuiAddLabel(subsub, 0,0, "This window have no buttons!\nAnd is inside a window inside a window!\nDispatching events from such structure...\nPleasure." );


  eoGuiAddButton( mainWin, 10,480-70,300,30, "Click me for great success!", &_btnGreatSuccess );

  guiLabel_s* lbl = eoGuiAddLabel( test.contextA, test.contextA->_size.x, test.contextA->_size.y-eoTxtHeight(FONT_LARGE), "GUI Test. Context A.");
  lbl->font = FONT_LARGE;
  lbl->fontPos = TXT_RIGHT;
  lbl->callback = &_testSetContext;
  lbl->callbackData = (void*)test.contextB;

  //
  guiWindow_s* testWin = eoGuiAddWindow(test.contextB, 0,0, 400,400, "Animated images inside windows.", BTN_SHOWCLOSE);

  lbl = eoGuiAddLabel( test.contextB, test.contextB->_size.x, test.contextB->_size.y-eoTxtHeight(FONT_LARGE), "GUI Test. Context B.");
  lbl->font = FONT_LARGE;
  lbl->fontPos = TXT_RIGHT;
  lbl->callback = &_testSetContext;
  lbl->callbackData = (void*)test.contextA;

  guiWindow_s* animWin = eoGuiAddWindow( testWin, 0,0, 150,150, "nothing", BTN_HIDECLOSE);
  animWin->showTitle = FALSE;
  eoGuiWinBgCol(animWin,0,0,0,0);
  eoGuiWinBorCol(animWin,0,0,0,0);
//  test.anim.img = eoGuiAddImage( animWin, 0,0, Data("/data/gfx/", "guianimtest.spr") );

  test.anim.btn = eoGuiAddButton( testWin, 170, 335, 60,30, "Stop!", &_testAnimate );

  testWin = eoGuiAddWindow(test.contextB, 500,500, 300,200, "Sounds and Music", BTN_SHOWCLOSE);
  eoGuiAddButton(testWin, 50,50, 200,20, "Copyright Infringement", _testSound)->callbackData=(void*)testWin;

  eoGuiContextSet( test.contextA );

  test.sound = eoSampleLoad( Data("/data/sound/", "test.wav") );
  test.music = eoMusicLoad( Data("/data/sound/","testmusic.mp3") );
  eoMusicFadeTo( test.music );
}

void _testFree()
{
  eoGuiContextDel( test.contextA );
  eoGuiContextDel( test.contextB );
  eoMusicStop( 250 );
  eoSampleFree( test.sound );
  eoMusicDel( test.music );
}

int guiTestConToggle( const char* str, void* data )
{

  if( str && atoi( str ) )
  {
    if( !eoGuiIsActive() )
    {
      eoGuiShow();
      eoGuiFade(GUI_FADE_IN, 500, NULL, NULL);
    }
    if( !test.showing )
      _testInit();
  } else {
    if( test.showing )
    {
      eoGuiContextSet( test.oldContext );
      _testFree();
      test.showing=FALSE;
    }
  }
  return(CON_CALLBACK_HIDE_RETURN_VALUE);
}
