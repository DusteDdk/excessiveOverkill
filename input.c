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

typedef struct {
    char* name;
    char* descr;
    inputCallback cb;
    int flags;
} bindableFunctionItem;

typedef struct {
    char* name;
    SDLKey key;
    inputCallback cb;
} bindableKeyItem;

int_fast8_t dispatchRunning;
listItem* delayedUnhook; //Since items might be unhooked while dispatching events, to avoid currupting the current iterator

listItem* allKeySubs; //List of subscribers that want all keys. (eventSubscriber_t)
listItem* keySubs; //List of keys with one or more subscribers (keySub_t)
listItem* keysDown; //List of keys that are currently pressed down. (keySub_t)

listItem* mouse; //For subscribers of mousemovement.
listItem* stick; //For subscribers of joystickactions.

listItem* bindableFunctions;
listItem* bindableKeys;

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
      eoPrint("Key ^3%i^1 (^2 %s ^1) bound to ^2%i^1 function(s)",(int)ks->key.sym ,SDL_GetKeyName(ks->key.sym), listSize( ks->subscribers ) );
    else
      eoPrint("ERROR: key %i ( %s ) is in keySubs but have empty subscriber list.", (int)ks->key.sym, SDL_GetKeyName(ks->key.sym)  );
  }
  if( listSize( allKeySubs ) )
  {
    eoPrint( "There are also ^2%i^1 functions that are bound to all keys.", listSize( allKeySubs ) );
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

int _inpListInFuncs(const char* str, void* data)
{
  char flagStr[32];
  listItem* it = bindableFunctions;
  bindableFunctionItem* t=NULL;
  eoPrint("%i functions can be bound to input:", listSize(bindableFunctions));
  while( (it=it->next) )
  {
    t=(bindableFunctionItem*)it->data;
    flagStr[0] = '\0';
    if( (t->flags&INPUT_FLAG_DOWN) )
    {
      sprintf( flagStr,"%s [down]", flagStr);
    }
    if( (t->flags&INPUT_FLAG_UP) )
    {
      sprintf( flagStr,"%s [up]", flagStr);
    }
    if( (t->flags&INPUT_FLAG_HOLD) )
    {
      sprintf( flagStr,"%s [hold]", flagStr);
    }

    eoPrint( "%s - (%s ) %s", t->name, flagStr, t->descr );
  }

  return(CON_CALLBACK_HIDE_RETURN_VALUE);
}

int _inpListInKeys(const char* str, void* data)
{
  listItem* it = bindableKeys;
  bindableKeyItem* k=NULL;

  eoPrint("%i bindable keys:", listSize(bindableKeys));
  while( (it=it->next) )
  {
    k = (bindableKeyItem*)it->data;
    eoPrint("%s", k->name );
  }
  return(CON_CALLBACK_HIDE_RETURN_VALUE);
}

int _inpBind(const char* str, void* data)
{
  bool found=0;
  listItem* it;
  bindableFunctionItem* fi;
  bindableFunctionItem* fii;
  bindableKeyItem* ki;

  //Parse arguments
  char keyName[128];
  char funName[128];
  splitVals( ' ', str, keyName, funName );

  //Check that key exist
  it=bindableKeys;
  while( (it=it->next) )
  {
    ki = (bindableKeyItem*)it->data;
    if( strcmp(ki->name, keyName ) == 0 )
    {
      found=1;
      break;
    }
  }

  if(!found)
  {
    eoPrint("The key '%s' is not a bindable key, type inkeys for a list of usable keys.");
  } else {
    it=bindableFunctions;
    found=0;
    while( (it=it->next) )
    {
      fi = (bindableFunctionItem*)it->data;
      if( strcmp(fi->name, funName ) == 0 )
      {
        found=1;
        break;
      }
    }

    if(!found)
    {
      eoPrint("The function '%s' is not a bindable function, type infuncs for a list of usable functions.");
    } else {

      if( ki->cb )
      {
        //Find name of current callback
        it=bindableFunctions;
        while( (it=it->next) )
        {
          fii = (bindableFunctionItem*)it->data;

          //Find the one we are bound to
          if( fii->cb == ki->cb )
          {

            //Already the same function?
            if( strcmp(fii->name, fi->name) == 0 )
            {
              eoPrint("Dude, %s is already bound to %s!", ki->name, fii->name);
              found=0;
            } else {
              eoPrint("Currently bound to %s, unbinding %s.", fii->name,keyName);
              eoInpRemHook( INPUT_EVENT_KEY, ki->key, ki->cb );
            }
            break;
          }
        }
      }

      if( found )
      {
        eoPrint("%s is now bound to %s", keyName, funName );
        ki->cb=fi->cb;
        eoInpAddHook( INPUT_EVENT_KEY, fi->flags, ki->key, fi->cb );
      }

    }

  }


  return(CON_CALLBACK_HIDE_RETURN_VALUE);
}

void eoInpAddFunc( const char* funcName, const char* funcDescr, inputCallback cb,int flags )
{

  if( !((flags&INPUT_FLAG_DOWN)||(flags&INPUT_FLAG_UP)||(flags&INPUT_FLAG_HOLD)) )
  {
    eoPrint("eoInpAddFunc( %s ) Error: One or more of INPUT_FLAG_[UP/DOWN/HOLD] required. Not added.", funcName);
    return;
  }

  if( ((flags&INPUT_FLAG_MOVEMENT)||(flags&INPUT_FLAG_EXCLUSIVE)) )
  {
    eoPrint("eoInpAddFunc( %s ) Error: INPUT_FLAG_[EXCLUSIVE/MOVEMENT] not allowed. Not added.", funcName);
    return;
  }

  bindableFunctionItem* t = malloc(sizeof(bindableFunctionItem));
  t->cb=cb;
  t->flags = flags;
  t->name=malloc(strlen(funcName)+1);
  strcpy( t->name, funcName );
  t->descr = malloc(strlen(funcDescr)+1);
  strcpy( t->descr, funcDescr);

  listAddData(bindableFunctions, (void*)t);
}

void _addValidInputKey( const char* name, SDLKey key )
{
  bindableKeyItem* t = malloc(sizeof(bindableKeyItem));
  t->key=key;
  t->name = malloc( strlen(name)+1 );
  t->cb=NULL;
  strcpy( t->name, name );

  listAddData(bindableKeys, (void*)t);
}

//Test-function to show how binding works.
void _inputTestBindFunction( inputEvent* e )
{
  eoPrint("Got input event!");
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
  bindableFunctions = initList();
  bindableKeys = initList();

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

  //Hook the infuncs (Lists functions that can be bound to a key)
  eoFuncAdd( _inpListInFuncs,NULL, "infuncs" );

  //Hook the inkeys (Lists the keys that can be bound to a function)
  eoFuncAdd( _inpListInKeys,NULL, "inkeys" );

  //Hook the bind function. (Binds a key to a function)
  eoFuncAdd( _inpBind,NULL, "bind" );

  //Add list of bindable keys
  _addValidInputKey( "0", SDLK_0);
  _addValidInputKey( "1", SDLK_1);
  _addValidInputKey( "2", SDLK_2);
  _addValidInputKey( "3", SDLK_3);
  _addValidInputKey( "4", SDLK_4);
  _addValidInputKey( "5", SDLK_5);
  _addValidInputKey( "6", SDLK_6);
  _addValidInputKey( "7", SDLK_7);
  _addValidInputKey( "8", SDLK_8);
  _addValidInputKey( "9", SDLK_9);

  _addValidInputKey( "a", SDLK_a);
  _addValidInputKey( "b", SDLK_b);
  _addValidInputKey( "c", SDLK_c);
  _addValidInputKey( "d", SDLK_d);
  _addValidInputKey( "e", SDLK_e);
  _addValidInputKey( "f", SDLK_f);
  _addValidInputKey( "g", SDLK_g);
  _addValidInputKey( "h", SDLK_h);
  _addValidInputKey( "i", SDLK_i);
  _addValidInputKey( "j", SDLK_j);
  _addValidInputKey( "k", SDLK_k);
  _addValidInputKey( "l", SDLK_l);
  _addValidInputKey( "m", SDLK_m);
  _addValidInputKey( "n", SDLK_n);
  _addValidInputKey( "o", SDLK_o);
  _addValidInputKey( "p", SDLK_p);
  _addValidInputKey( "q", SDLK_q);
  _addValidInputKey( "r", SDLK_r);
  _addValidInputKey( "s", SDLK_s);
  _addValidInputKey( "t", SDLK_t);
  _addValidInputKey( "u", SDLK_u);
  _addValidInputKey( "v", SDLK_v);
  _addValidInputKey( "w", SDLK_w);
  _addValidInputKey( "x", SDLK_x);
  _addValidInputKey( "y", SDLK_y);
  _addValidInputKey( "z", SDLK_z);
  _addValidInputKey( "up", SDLK_UP);
  _addValidInputKey( "down", SDLK_DOWN);
  _addValidInputKey( "left", SDLK_LEFT);
  _addValidInputKey( "right", SDLK_RIGHT);
  _addValidInputKey( "lctrl", SDLK_LCTRL);
  _addValidInputKey( "rctrl", SDLK_RCTRL);
  _addValidInputKey( "alt", SDLK_LALT);
  _addValidInputKey( "altgr", SDLK_RALT);
  _addValidInputKey( "space", SDLK_SPACE);
  _addValidInputKey( "shiftl", SDLK_LSHIFT);
  _addValidInputKey( "shiftr", SDLK_RSHIFT);
  _addValidInputKey( "backspace", SDLK_BACKSPACE);
  _addValidInputKey( "return", SDLK_RETURN);
  _addValidInputKey( "enter", SDLK_KP_ENTER);
  _addValidInputKey( "insert", SDLK_INSERT);
  _addValidInputKey( "home", SDLK_HOME);
  _addValidInputKey( "pageup", SDLK_PAGEUP);
  _addValidInputKey( "delete", SDLK_DELETE);
  _addValidInputKey( "end", SDLK_END);
  _addValidInputKey( "pagedown", SDLK_PAGEDOWN);
  _addValidInputKey( "*", SDLK_ASTERISK);
  _addValidInputKey( ",", SDLK_PERIOD);
  _addValidInputKey( ".", SDLK_PERIOD);
  _addValidInputKey( "f2", SDLK_F2);
  _addValidInputKey( "f3", SDLK_F3);
  _addValidInputKey( "f4", SDLK_F4);
  _addValidInputKey( "f5", SDLK_F5);
  _addValidInputKey( "f6", SDLK_F6);
  _addValidInputKey( "f7", SDLK_F7);
  _addValidInputKey( "f8", SDLK_F8);
  _addValidInputKey( "f9", SDLK_F9);
  _addValidInputKey( "f10", SDLK_F10);
  _addValidInputKey( "f11", SDLK_F11);
  _addValidInputKey( "f12", SDLK_F12);
  _addValidInputKey( "pause", SDLK_PAUSE);

  eoInpAddFunc("inptestfunup",   "This function gets called if a key is released.", _inputTestBindFunction, INPUT_FLAG_UP );
  eoInpAddFunc("inptestfundown", "This function gets called if a key is pressed down.", _inputTestBindFunction, INPUT_FLAG_DOWN );
  eoInpAddFunc("inptestfunboth", "This function gets called if a key is release or pressed.", _inputTestBindFunction, INPUT_FLAG_UP|INPUT_FLAG_DOWN );
  eoInpAddFunc("inptestfunhold", "This function gets called continuously when a key is held down.", _inputTestBindFunction, INPUT_FLAG_HOLD );
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
  listItem* iit;
  listItem* l;
  //TODO: Broken removal stuff
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
          it=listRemoveItem( l, it );
          _freeEventSubscriber( s );
          //Let's check if list is now empty
          if( !listSize( l ) )
          {
            //Free that list.
            freeList( l );

            //Remove that list from keySubs
            iit = keySubs;
            while( (iit=iit->next) )
            {
              if( iit->data == (void*)l )
              {
                iit=listRemoveItem( keySubs, iit );
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
