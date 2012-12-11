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

#include "input.h"
#include <SDL/SDL.h>
#include "tick.h"
#include "console.h"
#include "types.h"
typedef struct {
  void (*callback)(inputEvent*); //Function to be called
  inputEvent* e;
  int flags; //For detecting if we should callback on up/down/pressed and if it's exclusive
} eventSubscriber_t;

typedef struct {
  listItem* subscribers;
  SDL_keysym key;
  int timeDown;
} keySub_t;

typedef struct {
  void(*callback)(inputEvent*);
  listItem* l;
  uint16_t key; //To detect if it's a single key since those need to be removed from keys list aswell.
} delayedUnhookItem;

int_fast8_t dispatchRunning;
listItem* delayedUnhook; //Since items might be unhooked while dispatching events, to avoid currupting the current iterator

listItem* allKeySubs; //List of subscribers that want all keys. (eventSubscriber_t)
listItem* keySubs; //List of keys with one or more subscribers (keySub_t)
listItem* keysDown; //List of keys that are currently pressed down. (keySub_t)

listItem* mouse; //For subscribers of mousemovement.
listItem* stick; //For subscribers of joystickactions.

SDL_Joystick* joy[2];

SDL_MouseButtonEvent mouseState;

int inputShowBinds(const char* str, void* data)
{
  keySub_t* ks;
  listItem* itKs=keySubs;
  eoPrint("Bound keys:");
  while( (itKs=itKs->next) )
  {
    ks = (keySub_t*)itKs->data;
    if( ks->subscribers )
      eoPrint("Key ^3%i^1 (^2 %c ^1) bound to ^2%i^1 function(s)",(int)ks->key.sym ,ks->key.sym , listSize( ks->subscribers ) );
    else
      eoPrint("ERROR: key %i (%c) is in keySubs but have empty subscriber list.", (int)ks->key.sym,ks->key.sym  );
  }
  if( listSize( allKeySubs ) )
  {
    eoPrint( "There are also ^2%i^1 functions that are bound to every key.", listSize( allKeySubs ) );
  }

  if( listSize( mouse ) )
  {
    eoPrint( "There are also ^2%i^1 functions bound to mouseevents", listSize( mouse ) );
  }

  if( listSize( stick ) )
  {
    eoPrint( "There are also ^2%i^1 functions bound to joystickevents", listSize( stick ) );
  }

  return( CON_CALLBACK_HIDE_RETURN_VALUE );
}

void inputInit()
{
  dispatchRunning = 0;
  delayedUnhook = initList();
  allKeySubs = initList();
  keySubs = initList();
  keysDown = initList();
  mouse = initList();
  stick = initList();
  joy[0] = 0;
  joy[1] = 0;

  //Try and open joysticks.
  if( SDL_NumJoysticks() > 0 )
  {
    SDL_JoystickEventState(SDL_ENABLE);
    if( (joy[0]=SDL_JoystickOpen(0)) )
    {
      eoPrint("Joystick %s Opened as joy[0].", SDL_JoystickName(0));
      if( SDL_NumJoysticks() > 1 )
      {
        if( (joy[0]=SDL_JoystickOpen(1)) )
        {
          eoPrint("Joystick %s Opened as joy[1].", SDL_JoystickName(1));

        } else {
          eoPrint("Couldn't open joystick for joy[1].");
        }
      }
    } else {
      eoPrint("Couldn't open joystick for joy[0]");
    }

  } else {
    eoPrint("Not using joysticks. (none found)");
  }

}

int_fast8_t _checkExclusive(int flags, listItem* l)
{
  //Return 0 if we don't want it.
  if( !(flags&INPUT_FLAG_EXCLUSIVE) ) return(0);

  //Check if there's already an exclusive subscriber in list.
  if( l->next )
  {
    listItem* it = l->next;
    if(((eventSubscriber_t*)it->data)->flags&INPUT_FLAG_EXCLUSIVE)
    {
      //Return -1 if it's allready assigned.
      return(-1);
    }
  }

  //Return 1 if it's okay
  return(1);
}


//Search through list of keySub_t and return the list of eventSubscribers_t if any.
//Returns NULL if nothing is found.
listItem*  _getKeySubList(SDLKey key,listItem* l)
{
  keySub_t* s;
  listItem* it = l;
  while( (it=it->next) )
  {
    s = (keySub_t*)it->data;
    if( s->key.sym == key )
      return( s->subscribers );
  }
  return(NULL);
}

void _freeEventSubscriber( eventSubscriber_t* s )
{
  if( s->e )
  {
    if( s->e->key )
    {
      free(s->e->key);
    }

    if( s->e->mouse )
    {
      free(s->e->mouse);
    }

    if( s->e->stick )
    {
      free(s->e->stick);
    }
    free(s->e);
  }
  free(s);
}


void _addKeySubscriber( SDLKey key, int flags,  eventSubscriber_t* s, bool toAllKeys )
{
  int_fast8_t exclusive;
  listItem* l; // List of subscribers
  keySub_t* ks;
  s->e->key = malloc( sizeof( inputKey ) );

  //If it's to all keys, we know the list, it's allKeySubs
  if( toAllKeys )
  {
    l = allKeySubs;
  } else {
    //If it's not to all keys, we first find the list of subscribers.
    l = _getKeySubList(key, keySubs);
    //If it's not found, we create it.
    if(!l)
    {
      //No list found (no subscribers to key yet), add one.
      ks = malloc( sizeof(keySub_t) );
      ks->subscribers = initList();
      l = ks->subscribers;
      ks->key.sym = key;
      //Add keySub to keySubs
      listAddData( keySubs, (void*)ks );
    }
  }

  //Add subscriber to list of keys
  exclusive = _checkExclusive( flags, l );
  if( exclusive == 1 )
  {
    listInsertData( l, (void*)s, 0);
  } else if( exclusive == 0 )
  {
    listAddData( l, (void*)s );
  } else {
    eoPrint("eoInpAddHook Error: Key %i allready have an exclusive callback, and it's not going to be %i.", (int)key, s->callback);
    _freeEventSubscriber( s );
  }
}

void _addMouseSubscriber( int flags, eventSubscriber_t* s )
{
  int_fast8_t exclusive = _checkExclusive( flags, mouse );

  s->e->mouse = malloc( sizeof( inputMouse ) );

  if( exclusive==1 )
  {
    listInsertData( mouse, (void*)s, 0 );
  } else if(exclusive==0)
  {
    listAddData( mouse, (void*)s );
  } else {
    eoPrint("eoInpAddHook Error: Callback %i can't get exclusive rights to mouse, another subscriber have that.", s->callback );
    _freeEventSubscriber( s );
  }
}


void eoInpAddHook( int_fast8_t event, int flags, uint16_t key, void (*callback)(inputEvent*) )
{
  eventSubscriber_t* s = malloc(sizeof(eventSubscriber_t));
  s->flags = flags;
  s->callback=callback;
  s->e=malloc(sizeof(inputEvent));
  s->e->key = NULL;
  s->e->mouse = NULL;
  s->e->stick = NULL;

  switch( event )
  {
    case INPUT_EVENT_KEY:
      _addKeySubscriber( key, flags, s, FALSE );
    break;
    case INPUT_EVENT_ALL_KEYS:
      _addKeySubscriber( key, flags, s, TRUE );
    break;
    case INPUT_EVENT_MOUSE:
      _addMouseSubscriber( flags, s );
    break;
    case INPUT_EVENT_JOYSTICK:
      s->e->stick = malloc( sizeof( inputStick ) );
      listAddData( stick, (void*)s );
    break;
    default:
      eoPrint("Invalid eventtype %i", event);
      free(s->e);
      free(s);
    break;
  }
}

void _inputRemoveCallbackFromSubscriberList( listItem* l, void(*callback)(inputEvent*) )
{
  if(dispatchRunning)
  {
    delayedUnhookItem* un = malloc( sizeof( delayedUnhookItem ) );
    un->callback = callback;
    un->l = l;
    un->key = 0;
    eoPrint("Input callback queued for removal!");
    listAddData( delayedUnhook, (void*)un );
  } else {
    eventSubscriber_t* s;
    listItem* it=l;

    it=l;
    while( (it=it->next) )
    {
      s = (eventSubscriber_t*)it->data;
      if(callback == s->callback )
      {
        _freeEventSubscriber( s );
        it=listRemoveItem( l, it );
      }
    }
  }
}

void _inputRemoveSingleKeyCallback( uint16_t key, void(*callback)(inputEvent*) )
{
  eventSubscriber_t* s;
  listItem* it;
  listItem* l;

  l = _getKeySubList( key, keySubs );
  it = l;
  if( l )
  {
    if( dispatchRunning )
    {
      delayedUnhookItem* un = malloc( sizeof( delayedUnhookItem ) );
      un->callback = callback;
      un->l = 0;
      un->key = key;
      eoPrint("Input callback queued for removal!");
      listAddData( delayedUnhook, (void*)un );
    } else {
      while( (it=it->next) )
      {
        s = (eventSubscriber_t*)it->data;
        if(callback == s->callback )
        {
          listRemoveItem( l, it );
          _freeEventSubscriber( s );
          //Let's check if list is now empty
          if( !listSize( l ) )
          {
            //Free that list.
            freeList( l );

            //Remove that list from keySubs
            it = keySubs;
            while( (it=it->next) )
            {
              if( it->data == (void*)l )
              {
                listRemoveItem( keySubs, it );
              }
            }
          }
        }
      }
    }
  }
}

void _inputRemoveDelayed()
{
  if( listSize(delayedUnhook) )
  {
    listItem* it = delayedUnhook;
    delayedUnhookItem* un;
    while( (it=it->next) )
    {
      un = (delayedUnhookItem*)it->data;
      if( un->l )
      {
        _inputRemoveCallbackFromSubscriberList(un->l, un->callback);
        free(un);
        it = listRemoveItem(delayedUnhook, it);
      } else if( un->key )
      {
        _inputRemoveSingleKeyCallback( un->key, un->callback );
      }

    }
  }
}

void eoInpRemHook( int_fast8_t event, uint16_t key, void (*callback)(inputEvent*) )
{
  //Remove from all keys
  if( event == INPUT_EVENT_ALL_KEYS )
    _inputRemoveCallbackFromSubscriberList( allKeySubs, callback );

  //Remove from mouse
  if( event == INPUT_EVENT_MOUSE )
  {
    _inputRemoveCallbackFromSubscriberList( mouse, callback );
  }

  //Remove from stick
  if( event == INPUT_EVENT_JOYSTICK )
    _inputRemoveCallbackFromSubscriberList( stick, callback );


  //Remove from single key
  if( event == INPUT_EVENT_KEY )
  {
    _inputRemoveSingleKeyCallback(key, callback);
  }

}

//Returns TRUE if no more callbacks should be made (hit an exclusive subscribtion)
bool _keyCallBack( void* data, int keyFlag, SDL_keysym key, int timeDown )
{
  eventSubscriber_t* s = (eventSubscriber_t*)data;
  //Up/Down/Hold flags
  if( s->flags & keyFlag )
  {
    s->e->key->sym = key;
    s->e->key->timeDown = timeDown;
    (*s->callback)(s->e);

  }
  //Exclusive flag
  return( (s->flags & INPUT_FLAG_EXCLUSIVE) );
}

void inputRunKeys()
{
  bool exclusive = FALSE;

  dispatchRunning=1;
  listItem* itKd = keysDown, *it;
  keySub_t* ks;
  while( (itKd=itKd->next) )
  {
    ks = (keySub_t*)itKd->data;
    ks->timeDown+=eoTicks();

    //call subs on just this key
    it = ks->subscribers;
    if( it )
    {
      while( (it=it->next) )
      {
        if( _keyCallBack( it->data, INPUT_FLAG_HOLD, ks->key, ks->timeDown ) )
        {
          exclusive=TRUE;
          break;
        }
      }
    }

    if( !exclusive )
    {
      //Call subs on allkeys
      it = allKeySubs;
      while( (it=it->next) )
      {
        if( _keyCallBack( it->data, INPUT_FLAG_HOLD, ks->key, ks->timeDown ) == INPUT_FLAG_EXCLUSIVE )
        {
          break;
        }
      }
    }
  }

  //Mouse button down
  if( mouseState.state == SDL_PRESSED )
  {
    it = mouse;
    eventSubscriber_t* s;
    while( (it=it->next) )
    {
      s = (eventSubscriber_t*)it->data;
      if( s->flags&INPUT_FLAG_HOLD )
      {
        s->e->mouse->type = INPUT_EVENT_TYPE_BUTTON;
        s->e->mouse->button = mouseState;
        (*s->callback)(s->e);
        if( s->flags&INPUT_FLAG_EXCLUSIVE )
          break;
      }
    }
  }
  dispatchRunning=0;

  _inputRemoveDelayed();
}

void inputKeyDown( SDL_keysym k )
{
  keySub_t* ks = malloc( sizeof(keySub_t) );
  bool bound = FALSE;
  bool exclusive = FALSE;
  listItem* it;

  dispatchRunning=1;
  it = _getKeySubList( k.sym, keySubs );
  ks->key = k;
  ks->timeDown = 0;
  ks->subscribers = it; //Awesome
  listAddData( keysDown, ks );

  if( it )
  {
    bound = TRUE;
    while( (it=it->next) )
    {
      if( _keyCallBack( it->data,INPUT_FLAG_DOWN, k, 0 ) )
      {
        exclusive=TRUE;
        break;
      }
    }
  }

  if( !exclusive )
  {
    it = allKeySubs;
    while( (it=it->next) )
    {
      bound = TRUE;
      if( _keyCallBack( it->data,INPUT_FLAG_DOWN, k, 0 ) )
        break;
    }
  }


  if(!bound)
  {
    eoPrint("Key %i ('%c') unbound.", (int)k.sym, k.sym);
  }

  dispatchRunning=0;
}

void inputKeyUp( SDL_keysym k )
{
  dispatchRunning=1;
  bool exclusive = FALSE;
  listItem* it;
  keySub_t* ks = NULL;

  //Remove key from keysDown, free keySub_t later
  it = keysDown;
  while( (it=it->next) )
  {
    ks = (keySub_t*)it->data;
    if( ks->key.sym == k.sym )
    {
      listRemoveItem( keysDown, it );
      break;
    }
  }



  it = _getKeySubList( k.sym, keySubs );

  if( it )
  {
    while( (it=it->next) )
    {
      if( _keyCallBack( it->data,INPUT_FLAG_UP, k, ks->timeDown ) )
        break;
    }
  }

  if( !exclusive )
  {
    it = allKeySubs;
    while( (it=it->next) )
    {
      if( _keyCallBack( it->data,INPUT_FLAG_UP, k, ks->timeDown ) )
      {
        exclusive=TRUE;
        break;
      }
    }
  }

  //Free the keystruct
  free( ks );
  dispatchRunning=0;
}


void inputMouseMove( SDL_MouseMotionEvent motion )
{
  dispatchRunning=1;
  eventSubscriber_t* s;
  listItem* it = mouse;
  while( (it=it->next) )
  {
    s = (eventSubscriber_t*)it->data;
    if( s->flags & INPUT_FLAG_MOVEMENT )
    {
      s->e->mouse->type = INPUT_EVENT_TYPE_MOTION;
      s->e->mouse->motion = motion;
      (*s->callback)(s->e);
      if( s->flags & INPUT_FLAG_EXCLUSIVE )
        return;
    }
  }
  dispatchRunning=0;
}

void inputMouseButton( SDL_MouseButtonEvent button )
{
  dispatchRunning=1;
  eventSubscriber_t* s;
  listItem* it = mouse;
  mouseState = button;

  while( (it=it->next) )
  {
    s = (eventSubscriber_t*)it->data;
    s->e->mouse->type = INPUT_EVENT_TYPE_BUTTON;
    s->e->mouse->button = button;
    switch( button.state )
    {
      case SDL_PRESSED:
      if( s->flags&INPUT_FLAG_DOWN )
      {
        (*s->callback)(s->e);
        if( s->flags&INPUT_FLAG_EXCLUSIVE )
          return;
      }
      break;
      case SDL_RELEASED:
      if( s->flags&INPUT_FLAG_UP )
      {
        (*s->callback)(s->e);
        if( s->flags&INPUT_FLAG_EXCLUSIVE )
          return;
      }
      break;
    }
  }
  dispatchRunning=0;
}

void inputJoyMove( SDL_JoyAxisEvent motion )
{
  dispatchRunning=1;
  eventSubscriber_t* s;
  listItem* it;

  it=stick;
  while( (it=it->next) )
  {
    s = (eventSubscriber_t*)it->data;
    s->e->stick->type = INPUT_EVENT_TYPE_MOTION;
    s->e->stick->motion = motion;
    (*s->callback)(s->e);

  }
  dispatchRunning=0;
}

void _callJoyBtn( eventSubscriber_t* s, SDL_JoyButtonEvent button, int flag )
{
  dispatchRunning=1;
  if( s->flags & flag )
  {
    s->e->stick->type = INPUT_EVENT_TYPE_BUTTON;
    s->e->stick->button = button;
    (*s->callback)(s->e);
  }
  dispatchRunning=0;
}

void inputJoyButton( SDL_JoyButtonEvent button )
{
  dispatchRunning=1;
  eventSubscriber_t* s;
  listItem* it;

  it=stick;
  while( (it=it->next) )
  {
    s = (eventSubscriber_t*)it->data;

    if(button.state == SDL_PRESSED)
    {
      _callJoyBtn( s, button, INPUT_FLAG_DOWN );
    } else if( button.state == SDL_RELEASED )
    {
      _callJoyBtn( s, button, INPUT_FLAG_UP );
    }
  }
  dispatchRunning=0;
}
