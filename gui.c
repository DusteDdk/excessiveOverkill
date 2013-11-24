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

#include <math.h>
#include "gui.h"

#include "console.h"
#include "input.h"
#include "gltxt.h"
#include "gfxeng.h"
#include "gui-test.h"
#include "data.h"
#include "tick.h"
//Larger values are easier to see, smaller values allows more elements. should add to become 255.
#define GUI_COL_INC 15

#define _GUI_WINDOW_BORDER 3
//Prototypes
void guiRemoveWin(guiWindow_s* containerWin, guiWindow_s* removeWin);
void _guiDrawWin( guiWindow_s* win, GLfloat ofx, GLfloat ofy );

void _guiDrawScrollBar( guiScrollBar_s* sb, GLfloat ofx, GLfloat ofy );

typedef struct {
  bool drawGui;
  bool showCursor;
  bool isHooked;     //Check if we hooked mouse (since there are two functions that can do that)
  bool mBtnDown;      //State of mousebutton 1, to detect dragging a window
  vec2 mDragOffset;   //Offset from window pos (so window position can easily be set when dragging)
  guiElement_s* dragMe;//Object to be dragged, or NULL for no window.
  guiWindow_s* draggedParent; //To check we don't move outside it's parent window!
  sprite_s* cursor;
  vec2 cursorOffset;
  vec2 cursorPos;

  renderTex_t* renderTex;
  int useIdColor;    //When 1, elements will render with their unique identifier color instead.
  GLubyte idColsUsed[3];
  int showIdTex;  //For debug

  guiCallback fadeCallback;
  void* fadeCallbackData;
  int fadeState; //FADE_DONE when not fading, else it's in or out
  GLfloat fadeTimeLeft; //Ms
  GLfloat fadeTime;
  GLfloat fadeCol[4];
  GLuint _guiDrawStartList;
} gui_s;

static gui_s gui;

static guiWindow_s* activeContext;

void _guiBox( GLfloat x,GLfloat y, GLfloat w,GLfloat h )
{
  glDisable(GL_TEXTURE_2D);
  glBegin( GL_QUADS );
    glVertex2f( x,y+h );
    glVertex2f( x+w, y+h );
    glVertex2f( x+w, y );
    glVertex2f( x,y );
  glEnd();
}

void _guiBorder( GLfloat x, GLfloat y, GLfloat w, GLfloat h )
{
  glDisable(GL_TEXTURE_2D);
  glBegin( GL_LINE_LOOP );
    glVertex2f( x,y );
    glVertex2f( x+w, y );
    glVertex2f( x+w, y+h );
    glVertex2f( x, y+h );
  glEnd();
}


void _guiAddElement( guiWindow_s* dest, int type, void* data )
{
  guiElement_s* e = malloc( sizeof(guiElement_s) );
  e->type = type;
  e->data = data;

  if( gui.idColsUsed[0] == 255 )
  {
    gui.idColsUsed[0] = 0;
    if( gui.idColsUsed[1] == 255 )
    {
      gui.idColsUsed[1] = 0;
      if(gui.idColsUsed[2] == 255 )
      {
        eoPrint("GUI Error: No more unique identifiers, selection will not work for this element. Stop abusing the GUI!");
      } else {
        gui.idColsUsed[2] += GUI_COL_INC;
      }
    } else {
      gui.idColsUsed[1] += GUI_COL_INC;
    }
  } else {
    gui.idColsUsed[0] += GUI_COL_INC;
  }

  e->idCol[0] = gui.idColsUsed[0];
  e->idCol[1] = gui.idColsUsed[1];
  e->idCol[2] = gui.idColsUsed[2];
 // eoPrint("Element %i idColor %i %i %i",e, e->idCol[0],e->idCol[1],e->idCol[2] );

  listAddData( dest->elements, e );
}

guiWindow_s* eoGuiContextCreate()
{
  guiWindow_s* w = malloc( sizeof(guiWindow_s) );
  memset( w, 0, sizeof(guiWindow_s) );
  w->pos.x = 0;
  w->pos.y = 0;
  w->_size.x = (int)eoSetting()->res.x;
  w->_size.y = (int)eoSetting()->res.y;
  w->_draw = FALSE;
  w->showTitle = FALSE;
  w->showClose = FALSE;
  w->font = 0;
  w->fontPos = 0;
  w->title = NULL;
  w->movable = FALSE;
  w->callbackOnClose = 0;
  w->_packed = 0;
  w->elements = initList();
  return( w );
}


void _guiCloseBtnCallback(void* data)
{
  guiWindow_s* win = (guiWindow_s*)data;
  if((void*)win->callbackOnClose > BTN_SHOWCLOSE)
    (*win->callbackOnClose)(NULL);

  guiRemoveWin( (guiWindow_s*)win->_packed, win);
}

guiWindow_s* eoGuiAddWindow( guiWindow_s* container, int posx, int posy, int width, int height, const char* title, void* closeBehaviour)
{
  guiWindow_s* w = malloc( sizeof(guiWindow_s) );

  if( posx==GUI_POS_CENTER )
  {
    w->pos.x = container->_size.x/2 - width/2 - _GUI_WINDOW_BORDER;
  } else {
    w->pos.x = posx;
  }

  if( posy==GUI_POS_CENTER )
  {
    w->pos.y = container->_size.y/2 - height/2 - _GUI_WINDOW_BORDER;
  } else {
    w->pos.y = posy;
  }

  w->_size.x = width;
  w->_size.y = height;
  w->_draw = TRUE;
  w->showTitle = TRUE;
  w->font = FONT_MEDIUM;
  w->fontPos = TXT_LEFT;
  w->movable = TRUE;
  w->title = malloc( (strlen(title)+1)*sizeof(char) );
  strcpy( w->title, title );
  w->elements = initList();

  eoGuiWinBgCol( w, 0.2, 0.2, 0.2, 0.8 );
  eoGuiWinBorCol( w, 0.4, 0.4, 0.4, 0.9 );

  if(closeBehaviour > 0)
  {
    w->showClose = TRUE;
    w->callbackOnClose = closeBehaviour;
    eoGuiAddButton(w, width-20,-20, 10, 10, "X", &_guiCloseBtnCallback )->callbackData=(void*)w;
  } else {
    w->callbackOnClose = 0;
    w->showClose = FALSE;
  }

  w->_packed = (void*)container;

  //Add to container
  _guiAddElement(container, GUI_TYPE_WINDOW, (void*)w);


  return( w );
}

void eoGuiWinBgCol( guiWindow_s* w, GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
  w->colBg[0] = r;
  w->colBg[1] = g;
  w->colBg[2] = b;
  w->colBg[3] = a;
}

void eoGuiWinBorCol( guiWindow_s* w, GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
  w->colBorder[0] = r;
  w->colBorder[1] = g;
  w->colBorder[2] = b;
  w->colBorder[3] = a;
}

guiLabel_s* eoGuiAddLabel(guiWindow_s* container, GLfloat posx, GLfloat posy, const char* text )
{
  guiLabel_s* l = malloc( sizeof(guiLabel_s) );
  if( posx==GUI_POS_CENTER || posy==GUI_POS_CENTER)
  {
    eoPrint("Error: eoGuiAddLabel does not support center option yet.");
  }

  l->pos.y = posy;
  l->pos.x = posx;

  l->font = FONT_SMALL;
  l->fontPos = TXT_LEFT;
  l->callback = 0;
  l->callbackData = 0;
  l->txt = malloc( (strlen(text)+1)*sizeof(char) );
  strcpy( l->txt, text );

  _guiAddElement( container, GUI_TYPE_LABEL, (void*)l );

  return( l );
}

guiButton_s* eoGuiAddButton (guiWindow_s* container, GLfloat posx, GLfloat posy, GLfloat width, GLfloat height, const char* text, guiCallback callback )
{
  guiButton_s* b = malloc( sizeof(guiButton_s) );
  if( posx==GUI_POS_CENTER )
  {
    b->pos.x = container->_size.x/2 - width/2 - _GUI_WINDOW_BORDER;
  } else {
    b->pos.x = posx;
  }

  if( posy==GUI_POS_CENTER )
  {
    b->pos.y = container->_size.y/2 - height/2 - _GUI_WINDOW_BORDER;
  } else {
    b->pos.y = posy;
  }

  b->font = FONT_SMALL;
  b->fontPos = TXT_CENTER;
  b->size.x = width;
  b->size.y = height;
  b->txt = malloc( strlen(text)+1 );
  strcpy( b->txt, text );
  b->callback = callback;
  b->callbackData = 0;


  b->colBg[0] = 0.3;
  b->colBg[1] = 0.3;
  b->colBg[2] = 0.3;
  b->colBg[3] = 0.9;
  b->colBorder[0] = 0.5;
  b->colBorder[1] = 0.5;
  b->colBorder[2] = 0.5;
  b->colBorder[3] = 0.95;


  _guiAddElement( container, GUI_TYPE_BUTTON, (void*)b );

  return(b);
}

void eoGuiBtnSetTxt( guiButton_s* btn, const char* txt )
{
  if( btn->txt )
    free(btn->txt);
  btn->txt = malloc( (strlen(txt)+1)*sizeof(char) );
  strcpy( btn->txt, txt );
}

guiImage_s* eoGuiAddImage(guiWindow_s* container, GLfloat posx, GLfloat posy, const char* fileName )
{
  guiImage_s* img = malloc( sizeof(guiImage_s) );


  img->sprite = eoSpriteNew( eoSpriteBaseLoad(fileName), TRUE,TRUE ); ///TODO: Reuse the same sprite for multiple images?
  img->callback = 0;
  img->callbackData = 0;

  if( posx==GUI_POS_CENTER )
  {
    img->pos.x = container->_size.x/2 - img->sprite->base->frameSize.x/2 - _GUI_WINDOW_BORDER;
  } else {
    img->pos.x = posx;
  }

  if( posy==GUI_POS_CENTER )
  {
    img->pos.y = container->_size.y/2 - img->sprite->base->frameSize.y/2 - _GUI_WINDOW_BORDER;
  } else {
    img->pos.y = posy;
  }


  _guiAddElement( container, GUI_TYPE_IMAGE, (void*)img );
  return(img);
}

guiTextBox_s* eoGuiAddTextBox(guiWindow_s* container, GLfloat posx, GLfloat posy, int numLines, int font, const char* text)
{
  eoPrint("Scrollable textbox not implemented.");
  return(0);
}

guiScrollBar_s* eoGuiAddScrollBar( guiWindow_s* container, GLfloat posx, GLfloat posy, GLfloat width, GLfloat height, int type)
{
  guiScrollBar_s* sb = malloc( sizeof( guiScrollBar_s) );
  sb->_handleAt=0.0;
  sb->panSize=0.0;
  sb->minHandleSize=20.0;
  sb->colBg[0] = 0.8;
  sb->colBg[1] = 0.3;
  sb->colBg[2] = 0.3;
  sb->colBg[3] = 1.0;
  sb->colBorder[0] = 0.9;
  sb->colBorder[1] = 0.5;
  sb->colBorder[2] = 0.5;
  sb->colBorder[3] = 0.95;
  sb->size.x=width;
  sb->size.y=height;

  _guiAddElement( container, GUI_TYPE_SCROLLBAR, (void*)sb );
  return(sb);
}

void eoGuiContextSet( guiWindow_s* container )
{
  activeContext = container;
}

void _guiDestroyScrollBar( guiScrollBar_s* sb )
{
  free(sb);
}

void _guiDestroyWin( guiWindow_s* win );
void _guiDestroyLbl( guiLabel_s* lbl )
{
  //Dealloc text string
  free( lbl->txt );
  //Dealloc struct
  free( lbl );
}

void _guiDestroyBtn( guiButton_s* btn )
{
  //Dealloc text string
  free( btn->txt );
  //Dealloc struct
  free( btn );
}

void _guiDestroyImg( guiImage_s* img )
{
  //Dealloc sprite base
  eoSpriteBaseDel( img->sprite->base );
  //Dealloc sprite
  eoSpriteDel( img->sprite );
  //Dealloc struct
  free( img );
}

void _guiDestroyElements( listItem* list )
{
  listItem* it = list;
  guiElement_s* e;

  while( (it=it->next) )
  {
    e = (guiElement_s*)it->data;
    switch( e->type )
    {
      case GUI_TYPE_WINDOW:
        _guiDestroyWin( (guiWindow_s*)e->data );
      break;
      case GUI_TYPE_LABEL:
        _guiDestroyLbl( (guiLabel_s*)e->data );
      break;
      case GUI_TYPE_BUTTON:
        _guiDestroyBtn( (guiButton_s*)e->data );
      break;
      case GUI_TYPE_IMAGE:
        _guiDestroyImg( (guiImage_s*)e->data );
      break;
      case GUI_TYPE_SCROLLBAR:
        _guiDestroyScrollBar( (guiScrollBar_s*)e->data );
      break;
      default:
        eoPrint("Destruction of GUI type %i not yet implemented.", e->type);
      break;
    }

    //Free element itself.
    free(e);
  }
  //Free list
  freeList(list);

}

void _guiDestroyWin( guiWindow_s* win )
{
  if( win->title )
  {
    free(win->title);
  }

  _guiDestroyElements( win->elements );

  free(win);
}
//Destroys from a context by calling _guiDestroyWindow( win );
void eoGuiContextDel( guiWindow_s* container)
{
  if( container )
    _guiDestroyWin(container);
}

void _guiUnpack( guiWindow_s* container, guiWindow_s* remove )
{
    //Remove from containerWin's list
  listItem* it = container->elements;
  guiElement_s* e;

  eoPrint("Container listsize %i Before", listSize( container->elements ) );
  while( (it=it->next) )
  {
    e = (guiElement_s*)it->data;
    if( e->data == remove )
    {
      if(! listRemoveItem(container->elements, it))
        eoPrint("GUI Error: Couldn't unpack window %p from window &p!", remove);
        else
        eoPrint("GUI: Removing window %p since element %p->data = %p", remove, e, e->data );
      break;
    }
  }
  eoPrint("Container listsize %i After", listSize( container->elements ) );

}

void guiRemoveWin(guiWindow_s* containerWin, guiWindow_s* removeWin)
{
  eoPrint("Removing window %p", removeWin);
  //Remove from container
  _guiUnpack(containerWin, removeWin);

  //Call _guiDestroyWin
  _guiDestroyWin(removeWin);
}


void _guiDrawElements( listItem* l, GLfloat ofx, GLfloat ofy );

void _guiElementClicked( guiElement_s* e, SDL_MouseButtonEvent* btnEvent, guiWindow_s* parent)
{
  gui.mDragOffset.x = btnEvent->x;
  gui.mDragOffset.y = btnEvent->y;

  switch( e->type )
  {
    case GUI_TYPE_BUTTON:
    {
      guiButton_s* btn = (guiButton_s*)e->data;
      if(btnEvent->state==SDL_PRESSED)
      {
        btn->colBorder[0] = 1.0;
        btn->colBorder[1] = 1.0;
        btn->colBorder[2] = 1.0;
      } else {
        btn->colBorder[0] = 0.5;
        btn->colBorder[1] = 0.5;
        btn->colBorder[2] = 0.5;
        if( btn->callback )
        {
          (*btn->callback)(btn->callbackData);
        }
      }
    }
    break;
    case GUI_TYPE_WINDOW:
    { //Block needed to do scoped decleration of ptr var
      guiWindow_s* win = (guiWindow_s*)e->data;
      if(btnEvent->state==SDL_PRESSED && win->movable)
      {
        //Get it on top
        listRemoveByData( parent->elements, (void*)e );
        listAddData( parent->elements, e );
        gui.draggedParent = parent;
        gui.mDragOffset.x -= win->pos.x;
        gui.mDragOffset.y -= win->pos.y;
        gui.dragMe = e;
      }
    }
    break;
    case GUI_TYPE_SCROLLBAR:
      if( btnEvent->state==SDL_PRESSED )
      {
        guiScrollBar_s* sb = (guiScrollBar_s*)e->data;
        gui.mDragOffset.x -= sb->pos.x;
        gui.mDragOffset.y -= sb->pos.y + sb->_handleAt;
        gui.dragMe = e;
      }
    break;
    case GUI_TYPE_IMAGE:
      if( btnEvent->state==SDL_RELEASED )
      {
        guiImage_s* img = (guiImage_s*)e->data;
        if( img->callback )
        {
          (*img->callback)(img->callbackData);
        }
      }
    break;
    case GUI_TYPE_LABEL:
      if( btnEvent->state==SDL_RELEASED )
      {
        guiLabel_s* lbl = (guiLabel_s*)e->data;
        if( lbl->callback )
        {
          (*lbl->callback)(lbl->callbackData);
        }
      }
    break;
  }
}

void _guiFindElementByColor( guiWindow_s* win, GLubyte pix[3], SDL_MouseButtonEvent* btnEvent )
{
  listItem* it = win->elements;
  guiElement_s* e;

  while( (it=it->next) )
  {
    e = (guiElement_s*)it->data;

    if ( memcmp( pix, e->idCol, sizeof(GLubyte)*3 ) == 0 )
    {
      _guiElementClicked(e, btnEvent, win);
      //Not that more than one would be called, but if a window is removed, the iterator becomes invalid.
      return;
    } else if( e->type==GUI_TYPE_WINDOW )
    {
      _guiFindElementByColor( (guiWindow_s*)e->data, pix, btnEvent );
    }

  }
}

//We do it the easy way, and render everything in colors
void _guiFindClickedElement( SDL_MouseButtonEvent* btnEvent )
{
  GLubyte pix[3];

  if( activeContext )
  {
    //We draw not to the screen, but to oour texture
    eoGfxFboRenderBegin( gui.renderTex );
    eoGfxFboClearTex();

    //Tell all drawingcode to draw ID color blocks instead of actual ui
    gui.useIdColor=1;
    //Draw the elements
    _guiDrawWin( activeContext,0,0 );
    //Normal again
    gui.useIdColor=0;

    //Let's find the color of pixel clicked
    glReadPixels(btnEvent->x,eoSetting()->res.y-btnEvent->y,1,1,GL_RGB, GL_UNSIGNED_BYTE, &pix);

    //Stop renderingf to texture
    eoGfxFboRenderEnd();

    //Find the element that was clicked
    _guiFindElementByColor( activeContext, pix, btnEvent );
  }
}

void _guiMouseEvent( inputEvent* e )
{
  if( e->mouse )
  {
    if( e->mouse->type == INPUT_EVENT_TYPE_MOTION )
    {
      if( gui.mBtnDown )
      {
        //Let's drag a window! if there's one who wanna be dragged ;)
        if( gui.dragMe )
        {
          //Check that we don't drag it outside it's parent window
          int x = e->mouse->motion.x-gui.mDragOffset.x;
          int y = e->mouse->motion.y-gui.mDragOffset.y;

          if( gui.dragMe->type == GUI_TYPE_WINDOW )
          {
            guiWindow_s* win = (guiWindow_s*)gui.dragMe->data;
            int w = gui.draggedParent->_size.x -3;
            int h = gui.draggedParent->_size.y;
            if( gui.draggedParent->showClose || gui.draggedParent->showTitle )
            {
              h -= (eoTxtHeight( gui.draggedParent->font ) + 6 + 3 + 3);
            }
            if( (x > 3 && y > 3) && ( x+win->_size.x < w && y+win->_size.y < h) )
            {
              win->pos.x = x;
              win->pos.y = y;
            }
          } else if( gui.dragMe->type == GUI_TYPE_SCROLLBAR)
          {
            guiScrollBar_s* sb = (guiScrollBar_s*)gui.dragMe->data;

            sb->_handleAt = y;

            if( sb->_handleAt < 0 )
            {
              sb->_handleAt =0;
            } else
            if( sb->_handleAt+sb->_handleSize > sb->size.y)
            {
              sb->_handleAt = sb->size.y-sb->_handleSize;
            }

            GLfloat handle = sb->_handleSize;
            GLfloat freeScroll = sb->size.y - handle;
            GLfloat pan = sb->panSize;
            GLfloat pos = sb->_handleAt;

            GLfloat scrollablePanLeft = pan-sb->size.y;

            if(scrollablePanLeft < 0 ) scrollablePanLeft=0;

            GLfloat scale = scrollablePanLeft/sb->size.y;


            sb->panOffset = scale * sb->_handleAt;


            eoPrint("AtY: %f Offset: %f", sb->_handleAt, sb->panOffset );

          }
        }
      }
      gui.cursorPos.x = e->mouse->motion.x;
      gui.cursorPos.y = e->mouse->motion.y;
    } else if( e->mouse->type == INPUT_EVENT_TYPE_BUTTON)
    {
      if( e->mouse->button.button == 1 )
      {
        //Remember state (for dragging windows)
        if( e->mouse->button.state == SDL_PRESSED )
        {
          gui.mBtnDown = TRUE; //For detecting that mouse just touched down (dragging for instance, or markingg a button as clicked down)
        } else {
          gui.mBtnDown = FALSE;//For detecting that mouse released (for a button to reset and callback for instance)
          gui.dragMe = 0;
          gui.draggedParent = 0; //let's crash if someone tries to read it when tehre's nothign to be dragged
        }
        _guiFindClickedElement( &e->mouse->button );
      }
    }
  }
}

void _guiUnHookMouse()
{
  if( gui.isHooked )
  {
    eoInpRemHook( INPUT_EVENT_MOUSE, 0, &_guiMouseEvent );
    gui.isHooked=FALSE;
  }
}

void _guiHookMouse()
{
  if( !gui.isHooked )
  {
    eoInpAddHook( INPUT_EVENT_MOUSE, INPUT_FLAG_MOVEMENT|INPUT_FLAG_DOWN|INPUT_FLAG_UP, 0, _guiMouseEvent );
    gui.isHooked = TRUE;
  }
}

void eoGuiShow()
{
  if(!gui.drawGui)
  {
    gui.drawGui=TRUE;
    //Hook mouse events only if the mouse is shown
    if( gui.showCursor )
    {
      _guiHookMouse();
    }
  }
}

int eoGuiIsCursorVisible()
{
  return( gui.showCursor );
}

void eoGuiHide()
{
  if(gui.drawGui)
  {
    gui.drawGui=FALSE;
    _guiUnHookMouse();
  }
}

void _guiToggle( inputEvent* e )
{
  if( gui.drawGui )
    eoGuiHide();
  else
    eoGuiShow();
}

void guiInit()
{
  memset( &gui, 0x00, sizeof( gui_s ) );
  gui._guiDrawStartList = glGenLists(1);

  glNewList( gui._guiDrawStartList, GL_COMPILE );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho( 0, eoSetting()->res.x, eoSetting()->res.y, 0, 0,1);
    glColor4f(1,1,1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable( GL_CULL_FACE );
    glDisable(GL_LIGHTING);
  glEndList();

  activeContext = 0;
  //Load mouse cursor
  gui.cursor = eoSpriteNew( eoSpriteBaseLoad( Data("/data/gfx/", "cursor.spr") ), 1, 1 );
  gui.cursorPos.x = 0;
  gui.cursorPos.y = 0;
  gui.cursorOffset.x = 0;
  gui.cursorOffset.y = 0;

  //Hook toggleUi to esc key
  //eoInpAddHook( INPUT_EVENT_KEY, INPUT_FLAG_DOWN|INPUT_FLAG_EXCLUSIVE, SDLK_ESCAPE, &_guiToggle );

  gui.mBtnDown = FALSE;
  gui.mDragOffset.x = 0;
  gui.mDragOffset.y = 0;
  gui.dragMe = NULL;
  gui.idColsUsed[0] = 0.0;
  gui.idColsUsed[1] = 0.0;
  gui.idColsUsed[2] = 0.0;
  gui.useIdColor = 0;
  gui.showIdTex = 0;

  gui.renderTex = eoGfxFboCreate(eoSetting()->res.x,eoSetting()->res.y);

  eoVarAdd( CON_TYPE_INT,0, &gui.useIdColor, "gui-idcol" );
  eoVarAdd( CON_TYPE_INT,0, &gui.showIdTex, "gui-showidtex" );
  eoFuncAdd( &guiTestConToggle, NULL, "gui-test" );

  gui.isHooked=FALSE;
  eoGuiShowCursor(1);

  if( (255 % GUI_COL_INC) )
    eoPrint("GUI init error: GUI_COL_INC (%i) is wrong, have to mod(255, GUI_COL_INC) == 0.", GUI_COL_INC);


  gui.fadeCallback=NULL;
  gui.fadeState=GUI_FADE_DONE;
  gui.fadeTimeLeft=0;
  gui.fadeTime=0;
  gui.fadeCol[0]=0;
  gui.fadeCol[1]=0;
  gui.fadeCol[2]=0;



  eoPrint("GUI initialized, allowing a max of %i total elements.", (int)pow( (255.0/(float)GUI_COL_INC),3) );

  eoGuiFade(GUI_FADE_IN, 500, NULL, NULL);

}


void _guiDrawBtn( guiButton_s* b, GLfloat ofx, GLfloat ofy )
{
  GLfloat x = b->pos.x + ofx;
  GLfloat y = b->pos.y + ofy;

  if( gui.useIdColor )
  {
      _guiBox(x,y, b->size.x, b->size.y);
  } else {
      glColor4f( b->colBg[0],b->colBg[1],b->colBg[2],b->colBg[3] );
      _guiBox(x,y, b->size.x, b->size.y);
      glColor4f( b->colBorder[0],b->colBorder[1],b->colBorder[2],b->colBorder[3] );
      _guiBorder(x,y, b->size.x, b->size.y);
      glEnable(GL_TEXTURE_2D);

      eoTxtWrite( b->font, b->fontPos , b->txt, x+(b->size.x/2), y+ (b->size.y/2)-(eoTxtHeight(b->font)/2) );
  }
}

void _guiDrawImg( guiImage_s* img, GLfloat ofx, GLfloat ofy )
{
  vec2 p;
  p = img->pos;
  p.x += ofx;
  p.y += ofy;
  if( gui.useIdColor && img->callback )
  {
    _guiBox( p.x, p.y, img->sprite->base->spriteSize.x, img->sprite->base->spriteSize.y );
  } else if(!gui.useIdColor)
  {
    glEnable( GL_TEXTURE_2D );
    glColor4f(1,1,1,1);
    spriteDraw2D( img->sprite,p );
  }
}

void _guiDrawLbl( guiLabel_s* lbl, GLfloat ofx, GLfloat ofy )
{
  GLfloat x=lbl->pos.x+ofx;
  GLfloat y=lbl->pos.y+ofy;

  if( gui.useIdColor && lbl->callback)
  {
    GLfloat w = eoTxtLineWidth( lbl->font, lbl->txt, strlen(lbl->txt) );
    GLfloat h = eoTxtHeight( lbl->font );
    if(lbl->fontPos==TXT_CENTER) x=x-w/2.0;
    if(lbl->fontPos==TXT_RIGHT) x=x-w;
    _guiBox( x,y, w,h );
  } else if( !gui.useIdColor )
  {
    glEnable(GL_TEXTURE_2D);
    eoTxtWrite(lbl->font, lbl->fontPos, lbl->txt, x, y);
  }


}

void _guiDrawTxtBox( guiTextBox_s* txt, GLfloat ofx, GLfloat ofy )
{
  eoPrint("Scrollable textarea not implemented.");
}


void _guiDrawElements( listItem* l, GLfloat ofx, GLfloat ofy )
{
  listItem* it = l;
  guiElement_s* e;

  //It's up to the draw functions of each element to turn on their own coloring/texturing while we draw using idColors.

  while( (it=it->next) )
  {
    e = ((guiElement_s*)it->data);

    if( gui.useIdColor )
    {
      glColor4ub( e->idCol[0],e->idCol[1],e->idCol[2],255 );
    }
    glLoadIdentity();

    switch( e->type )
    {
      case GUI_TYPE_BUTTON:
        _guiDrawBtn( (guiButton_s*)e->data, ofx,ofy );
      break;
      case GUI_TYPE_IMAGE:
        _guiDrawImg( (guiImage_s*)e->data, ofx,ofy );
      break;
      case GUI_TYPE_LABEL:
        _guiDrawLbl( (guiLabel_s*)e->data, ofx,ofy );
      break;
      case GUI_TYPE_TEXTBOX:
        _guiDrawTxtBox( (guiTextBox_s*)e->data, ofx,ofy );
      break;
      case GUI_TYPE_WINDOW:
        _guiDrawWin( (guiWindow_s*)e->data, ofx,ofy );
      break;
      case GUI_TYPE_SCROLLBAR:
        _guiDrawScrollBar( (guiScrollBar_s*)e->data, ofx, ofy );
      break;
    }
  }
}

void _guiDrawWin( guiWindow_s* win, GLfloat ofx, GLfloat ofy )
{
  //Offsets from this window
  //We draw the window graphics (if any)
  if( win->_draw )
  {
    GLfloat x = win->pos.x+ofx;
    GLfloat y = win->pos.y+ofy;
    GLfloat w = win->_size.x;
    GLfloat h = win->_size.y;

    //Add border width/height to objects inside window.
    ofx += win->pos.x+_GUI_WINDOW_BORDER;
    ofy += win->pos.y+_GUI_WINDOW_BORDER;
    if( win->showTitle || win->showClose )
      ofy += eoTxtHeight( win->font )+6;

    if( gui.useIdColor )
    {
      //Drawing box representing window
      _guiBox( x,y,w,h );
    } else {
      //Drawing real window
      glDisable( GL_TEXTURE_2D );
      glColor4f( win->colBg[0],win->colBg[1],win->colBg[2],win->colBg[3] );
      _guiBox( x,y, w,h );
      glColor4f( win->colBorder[0],win->colBorder[1],win->colBorder[2],win->colBorder[3] );
      _guiBorder(x,y,w,h);

      if( win->showTitle )
      {
        glBegin(GL_LINES);
          glVertex2f( x+w, y+eoTxtHeight(win->font)+6 );
          glVertex2f( x, y+eoTxtHeight(win->font)+6 );
        glEnd();
        eoTxtWrite( win->font, win->fontPos, win->title, x+3, y+3 );
      }

    }
  }


  ///TODO:
  //Clear previous stencil
  //Create new stencil for window.

  //Then we draw the list of elements.
  _guiDrawElements( win->elements, ofx, ofy  );
}

void _guiDrawScrollBar( guiScrollBar_s* sb, GLfloat ofx, GLfloat ofy )
{
  GLfloat x = sb->pos.x+ofx;
  GLfloat y = sb->pos.y+ofy;
  GLfloat w = sb->size.x;
  GLfloat h = sb->size.y;

  sb->_handleSize = sb->size.y - (sb->panSize-sb->size.y);

  if( sb->_handleSize < sb->minHandleSize )
  {
    sb->_handleSize=sb->minHandleSize;
  } else if( sb->_handleSize > sb->size.y )
  {
    sb->_handleSize = sb->size.y;
  }

  if( gui.useIdColor )
  {
    _guiBox( x,y+sb->_handleAt,w,h );
  } else {
    glDisable( GL_TEXTURE_2D );
    glColor4f( sb->colBg[0],sb->colBg[1],sb->colBg[2],sb->colBg[3] );
    _guiBox( x,y+sb->_handleAt, w, sb->_handleSize );
    glColor4f( sb->colBorder[0],sb->colBorder[1],sb->colBorder[2],sb->colBorder[3] );
    _guiBorder(x,y,w,h);
  }

}

void guiDraw()
{
  //Set matrix correctly
  glCallList( gui._guiDrawStartList );

  if( gui.drawGui )
  {


    //Draw the windows
    if( activeContext )
    {
      glLoadIdentity();
      _guiDrawWin( activeContext,0,0 );
    }

    //Show the cursor
    if( gui.showCursor )
    {
      glEnable( GL_TEXTURE_2D );
      glColor4f( 1,1,1,1 );
      glPushMatrix();
      glLoadIdentity();
      vec2 curPos;
      curPos.x = gui.cursorPos.x + gui.cursorOffset.x;
      curPos.y = gui.cursorPos.y + gui.cursorOffset.y;
      spriteDraw2D( gui.cursor, curPos );
      glPopMatrix();
    }

    if( gui.showIdTex )
    {
      glLoadIdentity();
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, gui.renderTex->tex );
      glColor4f( 1,1,1, 0.5 );
      glBegin( GL_QUADS );
        glTexCoord2f( 0,gui.renderTex->t ); glVertex2f( 0,0 );
        glTexCoord2f( gui.renderTex->s, gui.renderTex->t ); glVertex2f( eoSetting()->res.x,0 );
        glTexCoord2f( gui.renderTex->s, 0 ); glVertex2f( eoSetting()->res.x, eoSetting()->res.y );
        glTexCoord2f( 0, 0 ); glVertex2f( 0, eoSetting()->res.y );
      glEnd();
    }

    //Fading?
    if( gui.fadeState != GUI_FADE_DONE )
    {
      if( gui.fadeState == GUI_FADE_IN )
        gui.fadeCol[3] = (gui.fadeTimeLeft/gui.fadeTime);
      else if( gui.fadeState == GUI_FADE_OUT )
        gui.fadeCol[3] = 1.0-(gui.fadeTimeLeft/gui.fadeTime);

      gui.fadeTimeLeft -= (GLfloat)eoTicks();
      if( gui.fadeTimeLeft < 1.0 )
      {
        gui.fadeState = GUI_FADE_DONE;
        if( gui.fadeCallback )
          gui.fadeCallback( gui.fadeCallbackData);
      }

      if( gui.fadeCol[3] != 0 )
      {
        glColor4fv( gui.fadeCol );
        _guiBox(0,0,eoSetting()->res.x, eoSetting()->res.y);
      }
    }


  }
  //Show console on top of everything?
  if(consoleVisible())
  {
    glPushMatrix();
    consoleRender();
    glPopMatrix();
  }

}

void eoGuiShowCursor( int showCursor )
{
  gui.showCursor=showCursor;

  //We only hook/unhook if gui is active
  if( gui.drawGui )
  {
    if( showCursor )
    {
      //We hook the mouse if we show the cursor
      _guiHookMouse();
    } else {
      //And if cursor is disabled, then we unhook
      _guiUnHookMouse();
    }
  }
}

void eoGuiSetCursor( sprite_s* spr, int pointX, int pointY )
{
	gui.cursorOffset.x = pointX;
	gui.cursorOffset.y = pointY;
	gui.cursor = spr;
}

void eoGuiWarpMouse( int16_t x, int16_t y)
{
  gui.cursorPos.x = x;
  gui.cursorPos.y = y;
  SDL_WarpMouse(x,y);
}

guiWindow_s* eoGuiContextGet()
{
  return( activeContext );
}

int eoGuiIsActive()
{
return( gui.drawGui );
}

int eoGuiFade(int action, int time, guiCallback callback, void* callbackData)
{
  if( action == GUI_FADE_QUERY )
    return( gui.fadeState );

  gui.fadeTime=gui.fadeTimeLeft=(GLfloat)time;

  gui.fadeCallback = callback;
  gui.fadeCallbackData = callbackData;

  gui.fadeState = action;

  return( action );
}
