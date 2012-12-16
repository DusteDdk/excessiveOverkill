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

#include "game.h"
#include "console.h"
#include "input.h"

static GLubyte idCols[4];
renderTex_t* gObjectSelectionTex;

static gameState_s state;
static int mouseLastX=0;
static int mouseLastY=0;
static int mouseDown=0;

void _gameTogglePause( inputEvent* e )
{
  state.isPaused = !state.isPaused;
  if( state.isPaused )
    eoPrint("Game is paused.");
  else
    eoPrint("Game no longer paused.");
}

int eoPauseGet()
{
  return( state.isPaused );
}

void eoPauseSet(int p)
{
  state.isPaused = p;
}

void _mouseEvent( inputEvent* e )
{
	if( e->mouse->type == INPUT_EVENT_TYPE_BUTTON)
	{
		mouseDown  = (e->mouse->button.state==SDL_PRESSED)?1:-1;
	} else {
		mouseLastX = e->mouse->motion.x;
		mouseLastY = e->mouse->motion.y;
	}
}


void eoGameInit()
{
  memset( &state, 0, sizeof(gameState_s) );
  eoPrint("gameInit();");

  state.isServer = 1;

  eoInpAddHook( INPUT_EVENT_KEY, INPUT_FLAG_DOWN, SDLK_PAUSE, &_gameTogglePause );
  eoVarAdd( CON_TYPE_INT, 0, &state.drawHitbox, "hitbox" );

  state.world.objs = initList();
  state._deleteObjs = initList();
  state.world.gameFrameStart = NULL;
  state.world.objSimFunc = NULL;
  state.world.initialized = 1;
  memset( idCols, 0, sizeof(GLubyte)*3);

  //Create a drawTexture
  gObjectSelectionTex = eoGfxFboCreate(eoSetting()->res.x,eoSetting()->res.y);

  //Add mouse hook
  eoInpAddHook( INPUT_EVENT_MOUSE, INPUT_FLAG_MOVEMENT|INPUT_FLAG_DOWN|INPUT_FLAG_UP, 0, _mouseEvent );

}

void _gameDeleteObj( listItem* delObjs)
{
  listItem* it=delObjs;
  engObj_s* obj;

  while( (it=it->next) )
  {
    obj=(engObj_s*)it->data;

    //If there's gamedata we will yell about it.
    //This is to avoid forgetting to free game specific data.
    //In most cases this data is unique to the object and needs freeing.
    if( obj->gameData != NULL )
      eoPrint("(game.c _gameDeleteObj):Possible memoryleak: Object %p have a gameData pointer to %p!", obj, obj->gameData );

    //First, we delete the objects components
    _gameDeleteObj( obj->components );

    //We remove the object form the world list.
    listRemoveByData( state.world.objs, it->data );

    //And we remove the object from the list
    it=listRemoveItem( delObjs, it );

    //If it's an emitter, we remove it
    if( obj->type==ENGOBJ_PAREMIT )
    {
      eoPsysFree( obj->emitter );
    }

    //Then we delete the object itself
    free(obj);

  }
}

void eoWorldClear()
{

  //Free world object list
  listItem* it = state.world.objs;
  while( (it=it->next) )
  {
    eoObjDel( (engObj_s*)it->data );
  }
  _gameDeleteObj( state._deleteObjs );

  freeList( state.world.objs );
  freeList( state._deleteObjs );

  //Set 0 so we can detect that it's freed.
  state.world.objs = 0;
  state.nextObj = 0;
  state.world.initialized = 0;

  //Free the drawTexture
  eoGfxFboDel( gObjectSelectionTex );

  //remove mouse hook
  eoInpRemHook( INPUT_EVENT_MOUSE, 0, _mouseEvent );


}

void _gameRunObject(listItem* objList)
{
  listItem* it=objList;
  engObj_s* obj;
  while( (it=it->next) )
  {
    obj=(engObj_s*)it->data;

    //If we're the server, we do cool stuff
    if( state.isServer )
    {
      if(obj->thinkFunc)
        obj->thinkFunc(obj);

	  //If the client code wants to be called with every object.
      if( state.world.objSimFunc )
      	state.world.objSimFunc(obj);

    }

    //Movement is simulated seperately so stuff keeps moving even if packages does not come every frame
    gameSimMovement(obj);

    //Now we simulate the attached objects if any
    if( obj->components->next ) //Way faster than listSize
    {
      _gameRunObject( obj->components );
    }

  }

}

int _gameBoxCollision(engObj_s* a, engObj_s* b)
{
  GLfloat aleft, aright, atop, abot, bleft,bright,btop,bbot;
  GLfloat afront, aback, bfront, bback;

  aleft = a->pos.x-a->_hitBox.x;
  aright= a->pos.x+a->_hitBox.x;

  atop = a->pos.z-a->_hitBox.z;
  abot = a->pos.z+a->_hitBox.z;

  bleft = b->pos.x-b->_hitBox.x;
  bright= b->pos.x+b->_hitBox.x;

  btop = b->pos.z-b->_hitBox.z;
  bbot = b->pos.z+b->_hitBox.z;

  afront = a->pos.y+a->_hitBox.y;
  aback = a->pos.y-a->_hitBox.y;

  bfront = b->pos.y+b->_hitBox.y;
  bback = b->pos.y-b->_hitBox.y;

  if( afront < bback ) return(0);
  if( aback > bfront ) return(0);

  if( abot < btop ) return(0);
  if( atop > bbot ) return(0);

  if( aright < bleft ) return(0);
  if( aleft > bright ) return(0);

  return(1);
}

void _gameRunCollisions()
{
  listItem* it=state.world.objs;
  engObj_s* obj;

  listItem* checkIt;
  engObj_s* checkObj;
  while( (it=it->next) )
  {
    obj=(engObj_s*)it->data;

    if( obj->colTeam )
    {
      //Check against every object on diffrent team
      checkIt=state.world.objs;
      while( (checkIt=checkIt->next) )
      {
        checkObj = (engObj_s*)checkIt->data;
        if( checkObj->colTeam && checkObj->colTeam != obj->colTeam )
        {
          //These two can collide, do stupid simple detection
          if( _gameBoxCollision(obj, checkObj) )
          {
            if( obj->colFunc )
              obj->colFunc(obj, checkObj);
          }
        }
      }
    }
  }
}


void gameRun()
{

  if( !state.world.objs ) return;

  if( state.world.gameFrameStart )
  {
	  state.world.gameFrameStart();
  }

  //If we're not paused, we simulate the world
  if( !state.isPaused )
  {
    //For each object in list
    _gameRunObject(state.world.objs );

    //Delete objects queued for deletion
    _gameDeleteObj( state._deleteObjs);

    //Now we do collision detection on the world as it looks this frame
    if( state.isServer )
      _gameRunCollisions();

    //We simulate particle systems
    psysSim();

  }

  //Draw objects
  gameDraw(state.world.objs);

  //Find any elements with mouse over them
  GLubyte pix[3];
  eoGfxFboRenderBegin( gObjectSelectionTex );
  glReadPixels( mouseLastX, eoSetting()->res.y-mouseLastY,1,1,GL_RGB, GL_UNSIGNED_BYTE, &pix);
  eoGfxFboClearTex();
  eoGfxFboRenderEnd();

  //Find obj same color as pix
  listItem* it=state.world.objs;
  engObj_s* obj;
	while( (it=it->next) )
	{
		obj = (engObj_s*)it->data;
		if( obj->clickedFunc )
		{
			if ( memcmp( pix, obj->_idcol, sizeof(GLubyte)*3 ) == 0 )
			{
				obj->clickedFunc( obj, mouseDown );
				break; //break the while loop, we can only hit one
			}
		}
	}

	mouseDown=0;
  //Draw particle systems
  psysDraw();

}

void eoRegisterSimFunc( void (*objSimFunc)(engObj_s*) )
{
	state.world.objSimFunc = objSimFunc;
}
void eoRegisterStartFrameFunc( void (*startFrameFunc)(void) )
{
	state.world.gameFrameStart = startFrameFunc;
}

engObj_s* eoObjCreate(int type)
{
  state.nextObj++;

  engObj_s* obj = malloc( sizeof(engObj_s) );
  memset( obj, 0, sizeof(engObj_s) );

  obj->type = type;
  obj->id = state.nextObj;
  obj->components = initList();
  obj->gameData = NULL;
  return(obj);
}

void eoObjBake(engObj_s* obj)
{
  if( obj->_baked )
  {
    eoPrint("Object %i already baked.", obj->id);
    return;
  }

  switch(obj->type)
  {
    case ENGOBJ_MODEL:
    //Check data
    if( obj->model )
    {
      //Set hitbox
      obj->_hitBox = obj->model->size;

      //Set "onclick" callback if any.
      if( obj->clickedFunc )
      {
		  //Todo: it's easily possible to get many more colors
    	  idCols[0] += 10;
    	  if( idCols[0] > 240)
    	  {
    		  idCols[1] += 10;
    		  if( idCols[1] > 240 )
    		  {
    			  idCols[2] += 10;
    			  if( idCols[2] > 240 )
    			  {
    				  eoPrint("Error: No free IdColors for object %i unsetting clickFunc.", obj->id );
    				  obj->clickedFunc=NULL;
    			  }
    		  }
    	  }
    	  if( obj->clickedFunc )
    	  {
    		  obj->_idcol[0]=idCols[0];
    		  obj->_idcol[1]=idCols[1];
    		  obj->_idcol[2]=idCols[2];
    	  }
      } //clickedFunc set

    } else {
      eoPrint("Object %i have no model, not baking.", obj->id );
      return;
    }
    break;

    case ENGOBJ_SPRITE:
    //Check data
    if( obj->sprite )
    {
      eoSpriteScale( obj->sprite, 0.01, 0.01 );
      //Set hitbox
      //We divide by 2 because middle of 3D objects are 0,0,0
      obj->_hitBox.y = 0.01;
      obj->_hitBox.x = (obj->sprite->base->spriteSize.x * obj->sprite->scale.x);
      obj->_hitBox.z = (obj->sprite->base->spriteSize.y * obj->sprite->scale.y);
    } else {
      eoPrint("Object %i have no sprite, not baking.", obj->id );
      return;
    }

    break;

    case ENGOBJ_PAREMIT:
    //Check data
    if( obj->emitter && !obj->emitter->_maxParticles  )
    {
      eoPrint("Object %i have no emitter, or it's emitter is not baked.", obj->id);
      return;
    }
		obj->_hitBox.x = 0.5;
		obj->_hitBox.y = 0.5;
		obj->_hitBox.z = 0.5;

    break;

    case ENGOBJ_SOUND:
    //Check data
    if( !obj->sound )
    {
      eoPrint("Object %i have no sound. Not adding");
      return;
    }
    break;
  }


  obj->_baked=1;
}

void eoObjAdd(engObj_s* obj)
{
  if( obj->_baked )
    listAddData( state.world.objs, (void*)obj );
  else
    eoPrint("Object %i not baked.", obj->id);
}

void eoObjAttach( engObj_s* parent, engObj_s* child )
{
  listAddData( parent->components, (void*)child );
  child->parent = parent;
}

void eoObjDel(engObj_s* obj)
{
  //This way we make sure we don't try to delete the same object twice-
  if( !obj->deleteMe )
  {
    obj->deleteMe=TRUE;
    listAddData( state._deleteObjs, (void*)obj );
  }
}

void gameSimMovement(engObj_s* obj)
{
  obj->pos.x += obj->vel.x;
  obj->pos.y += obj->vel.y;
  obj->pos.z += obj->vel.z;


  //If this object's got a parent, we update it's position
  if( obj->parent )
  {
    obj->pos.x = obj->parent->pos.x + obj->offsetPos.x;
    obj->pos.y = obj->parent->pos.y + obj->offsetPos.y;
    obj->pos.z = obj->parent->pos.z + obj->offsetPos.z;

    obj->rot.x = obj->parent->rot.x + obj->offsetRot.x;
    obj->rot.y = obj->parent->rot.y + obj->offsetRot.y;
    obj->rot.z = obj->parent->rot.z + obj->offsetRot.z;
  }

  //If this object is an emitter
  if( obj->type == ENGOBJ_PAREMIT )
  {
    obj->emitter->position = obj->pos;
    obj->emitter->emitDirection = eoVec3Normalize(obj->rot);
  }
}

void gameDraw(listItem* objList)
{
  listItem* it=objList;
  engObj_s* obj;

  if( !state.world.initialized ) return;

  while( (it=it->next) )
  {
    obj=(engObj_s*)it->data;

    gameDraw( obj->components );

    glPushMatrix();
    glTranslatef( obj->pos.x, obj->pos.y, obj->pos.z );
    glRotatef( obj->rot.x, 1,0,0 );
    glRotatef( obj->rot.y, 0,1,0 );
    glRotatef( obj->rot.z, 0,0,1 );

    //Draw hitbox
    if( state.drawHitbox )
    {
      glDisable(GL_TEXTURE_2D);
      glEnable(GL_COLOR_MATERIAL);
      glDisable( GL_LIGHTING );

      glLineWidth( 2 );
      glBegin( GL_LINES );
        glColor4f( 1,0,0,1 );
        glVertex3f( -obj->_hitBox.x, obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( obj->_hitBox.x, obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( -obj->_hitBox.x, -obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( obj->_hitBox.x, -obj->_hitBox.y, -obj->_hitBox.z );

        glVertex3f( -obj->_hitBox.x, obj->_hitBox.y, obj->_hitBox.z );
        glVertex3f( obj->_hitBox.x, obj->_hitBox.y, obj->_hitBox.z );
        glVertex3f( -obj->_hitBox.x, -obj->_hitBox.y, obj->_hitBox.z );
        glVertex3f( obj->_hitBox.x, -obj->_hitBox.y, obj->_hitBox.z );

        glColor4f( 0,0,1,1 );
        glVertex3f( obj->_hitBox.x, obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( obj->_hitBox.x, obj->_hitBox.y, obj->_hitBox.z );
        glVertex3f( obj->_hitBox.x, -obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( obj->_hitBox.x, -obj->_hitBox.y, obj->_hitBox.z );

        glVertex3f( -obj->_hitBox.x, obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( -obj->_hitBox.x, obj->_hitBox.y, obj->_hitBox.z );
        glVertex3f( -obj->_hitBox.x, -obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( -obj->_hitBox.x, -obj->_hitBox.y, obj->_hitBox.z );

        glColor4f( 0,1,0,1 );
        glVertex3f( obj->_hitBox.x, obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( obj->_hitBox.x, -obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( -obj->_hitBox.x, obj->_hitBox.y, -obj->_hitBox.z );
        glVertex3f( -obj->_hitBox.x, -obj->_hitBox.y, -obj->_hitBox.z );

        glVertex3f( obj->_hitBox.x, obj->_hitBox.y, obj->_hitBox.z );
        glVertex3f( obj->_hitBox.x, -obj->_hitBox.y, obj->_hitBox.z );
        glVertex3f( -obj->_hitBox.x, obj->_hitBox.y, obj->_hitBox.z );
        glVertex3f( -obj->_hitBox.x, -obj->_hitBox.y, obj->_hitBox.z );
      glEnd();

      glDisable(GL_COLOR_MATERIAL);
      glEnable( GL_LIGHTING );
    }

    GLubyte black[3];
    black[0]=0;
    black[1]=0;
    black[2]=0;
    switch(obj->type)
    {
      case ENGOBJ_MODEL:
    	    eoGfxFboRenderBegin( gObjectSelectionTex );
    	    if( obj->clickedFunc )
    	    	drawClayModel( obj->model, obj->_idcol );
    	    //else
    	    //	drawClayModel( obj->model, black ); //to potentially block.

    	    eoGfxFboRenderEnd();

    	  drawModel(obj->model);
      break;

      case ENGOBJ_SPRITE:
        glPushMatrix();
        eoGfxBillboardBegin();
        glEnable(GL_TEXTURE_2D);
        glDisable( GL_LIGHTING );
        glColor4f( 1.0,1.0,1.0,1.0 );
        spriteDraw( obj->sprite );
        glEnable( GL_LIGHTING );
        eoGfxBillBoardEnd();
        glPopMatrix();
      break;

      case ENGOBJ_SOUND:
        eoSamplePlay( obj->sound, 128 ); ///Fixme; should be relative to the camera position.
        eoObjDel( obj );
      break;
    }

    glPopMatrix();

  }
}

