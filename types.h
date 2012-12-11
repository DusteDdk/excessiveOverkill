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

//FIXME: This is idiotic, need to move everything back to their respective files.

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED
#include <string.h>

#include "gl.h"
#include "list.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>


/*******************************************************************************
  General structures and definitions
*******************************************************************************/

#define FALSE 0
#define TRUE 1

typedef int_fast8_t bool; //Don't really care about memory usage, just speed.
typedef struct { GLfloat x,y,z; } vec3; //3D vector
typedef struct { GLfloat x,y; } vec2;   //2D vector

typedef struct { int x; int y; } pointi;

/*******************************************************************************
  Settings structure
*******************************************************************************/
typedef struct {
  pointi res;
  GLfloat aspect;
  int fullScreen;
} settings_t;

/*******************************************************************************
Camera structures and definitions
*******************************************************************************/
#define CAM_PLAYSTATE_STOPPED  0
#define CAM_PLAYSTATE_PLAYING 1
#define CAM_PLAYSTATE_RECORDING 2
#define CAM_PLAYBACK_POSITION_ABSOLUTE 0
#define CAM_PLAYBACK_POSITION_RELATIVE 1
typedef struct {
  vec3 pos;     //Camera position
  vec3 target;  //Camera target position
  GLfloat zoom; //Camera zooming
} camData;

/*******************************************************************************
Console definitions
*******************************************************************************/
#define CON_TYPE_INT 0
#define CON_TYPE_STRING 1
#define CON_TYPE_FLOAT 2
#define CON_TYPE_FLOAT_ARRAY 3
#define CON_TYPE_VEC2 4
#define CON_TYPE_VEC3 5
#define CON_INTERNAL_TYPE_FUNC 6
#define CON_CALLBACK_HIDE_RETURN_VALUE -2193

/*******************************************************************************
  Game list entry
*******************************************************************************/
typedef struct {
  char* gameDir;
  char* gameName;
  char* gameDesc;
} gameListEntry_s;

/*******************************************************************************
  Model structure
*******************************************************************************/
typedef struct {
  //Since our faces are sorted after material, we only need one group pr mat.
  char* name;           //Name of material, used by int getMaterialIndex(matProps* array, char* name);
  GLuint matDL; //Displaylist of material
  GLuint start; //Number of triangles into the buffer where this material starts
  GLuint count;  //Number of triangles affected
  GLenum cubemap;
} modelMaterial;

typedef struct {
  char* name;
  GLuint bufferName;
  GLuint vaoName;
  GLenum drawType; //Triangles, points, so on

  char* textureName; //Just because I can..
  GLuint texture; //Model texture

  modelMaterial* materials;

  int matCount;         //Number of materials (thus, chunks to render, and displaylists)
  GLsizei vertexDataSize;
  GLsizei vertexCount;

  vec3 size;
  int recieveLight;
} vboModel;

/*******************************************************************************
  Sprite structures
*******************************************************************************/
typedef struct {
  vec2 spriteSize;
  vec2 frameSize;
  int animNumFrames;
  GLuint tex;
  int animSpeed;
  GLuint dl;
} sprite_base;

typedef struct {
  vec2 scale;
  sprite_base* base;
  int animFrame;
  int animTicks;
  bool animPlaying;
  int animLoop;
} sprite_s;

/*******************************************************************************
  Sound and music structures
*******************************************************************************/
typedef struct  {
  Mix_Chunk* snd;
  char* name;
} sound_s;

typedef struct {
  Mix_Music* mus;
  char* name;
} music_s;

/*******************************************************************************
  Particle system structures and definitions
*******************************************************************************/
//Outputs particles in some direction, spread in some angle, for engines and muzzle flashes
#define PAR_EMIT_CONE 1
//Outputs particles chaotically in every direction, for explosions and the likes
#define PAR_EMIT_EXPLO 2
//Outputs particles in a perfect circle around the Y axis, for effects
#define PAR_EMIT_RING 3
//Outputs particles in a perfect sphere, for effects
#define PAR_EMIT_SPHERE 4
//Outputs particles in a straight line, of length, effects. When this is in effect, angle is not angle, but endingpoint in space, where particles are drawn to and from
//Particles have no velocity
#define PAR_EMIT_LINE 5

typedef struct {
  int life;         //Amount of life from the start
  int lifeLeft;     //Life left before dying ( life/lifeLeft = fadeValue )
  GLfloat velocity[3];    //Added to position
  GLfloat position[3];    //Current position in space
  GLfloat rotation[3];    //Rotation of sprite
  GLfloat color[4]; //Color of this particle
  GLfloat size;
  sprite_s* sprite; //The sprite shown
} particle_s;

typedef struct {
  int rotateParticles;          //If 1, will randomly rotate particles (default on)
  sprite_base* sprBase;         //Sprite base
  int addictive;                //Use addictive blending
  int numParticlesPerEmission;  //How many particles do we emit each time
  int ticksBetweenEmissions;    //How many ticks between emissions, must be above 0, a negative value emits once, then resets to 0.
  int emitTimeVariance;         //Subtracts %rand(emitVariance)
  int particleLifeMax;          //The maximal lifespan of a single particle
  int particleLifeVariance;     //The variance on lifespan, (particle life is particleLifeAverage -random integer between 0 and particleLifeVariance
  int emitType;                 //Type of system to emit
  int percentFlicker;           //Amount of flickering (fading up and down randomly), 100 = never flicker, 0 = never visible 50 visible 50% of time
  int fade;                     //1 = Particle will fade out before dying, 0 = particles don't change their alpha.
  int shrink;                   //1 = particles will shrink in size before dying, 0 = particles stay the same size.
  GLfloat sizeMax;              //Max size of particles
  GLfloat sizeVariance;         //particleSize is calculated sizeMax-random(sizeVariance)
  GLfloat color[4];             //Particles of this color is drawn first
  GLfloat colorVariance[4];     //Colors are calculated color - ramdom GLfloat between 0 and variance
  vec3 position;                //Position of emitter, updated by object in sim code
  vec3 emitDirection;           //The general emit direction, a normalized cartesian vector
  vec3 wind;                    //For lack of better word, particles are affected in this direction 0,0,0 = not affected. Effect is cm per millisecond
  GLfloat emitAngle;            //The angle of emitting, particles are emitted in direction -angle/2 to angle/2
  GLfloat emitSpeedMax;         //The maximum speed a particle will be travelling at, speed-random(speed variance). Speed is centimeters pr millisecond
  GLfloat emitSpeedVariance;    //Variance in speed (negative)
  int _maxParticles;            //This is a function of particleLifeAverage and numEmissionsPerSecond, used to declare array large enough for all particles in worst case.
  int _timeToNextEmit;          //Countdown to next emit
  int _availParticles;          //Number of free slots
  particle_s* _particles;       //Array of particles in system


} particleEmitter_s;

/*******************************************************************************
  Game world and object structures and definitions
*******************************************************************************/
#define ENGOBJ_FREE 0
#define ENGOBJ_MODEL 1
#define ENGOBJ_SPRITE 2
#define ENGOBJ_PAREMIT 3
//Sounds are destroyed from world after they are played
#define ENGOBJ_SOUND 4


//typedef void (*engObjCallback_f)(engObj_s*);  //Called on each object if present
//typedef void (*engObjColCallback_f)(engObj_s*, engObj_s*);  //Called upon collision with object if present. First argument is the object that was hit, the second the one it hit.


typedef struct engObj_s {

  int id;
  int deleteMe;
  int type; //Sprite, Model, Particle Emitter or Sound clip
  int colTeam; //0 = no collision. >0 = collide with all on other team.
  int _baked; //0 = not yet baked
  vec3 pos;
  vec3 vel;
  vec3 rot;

  void* gameData; //Can be used by gamecode to store additional informations about the object.

  vec3 _hitBox; //Is calculated when adding to engine, from model/sprite.
  vec3 offsetPos; //For children, relative to their parents
  vec3 offsetRot; //For children, relative to their parents

  vboModel* model;
  sprite_s* sprite;
  sound_s* sound;
  particleEmitter_s* emitter;

  void (*thinkFunc)(struct engObj_s*);
  void (*colFunc)(struct engObj_s*,struct engObj_s*);

  listItem* components; //Any attached engine objects
  struct engObj_s* parent; //0 if this is the top
} engObj;


typedef struct {
  listItem* bgObjs;
  listItem* objs;
} world_s;

/*******************************************************************************
  Input system structures, definitions
*******************************************************************************/

//Subscribe to one key
#define INPUT_EVENT_KEY 1
//Callback when it's clicked first time only
//Subscribe to any keypress
#define INPUT_EVENT_ALL_KEYS 2
//Subscribe to any joystick event
#define INPUT_EVENT_JOYSTICK 4
//Subscribe to any mouse event
#define INPUT_EVENT_MOUSE 8

//Callback on key/button down
#define INPUT_FLAG_DOWN 1
//Callback on key/button up
#define INPUT_FLAG_UP 2
//Callback on key/button hold (every iteration)
#define INPUT_FLAG_HOLD 4
//Don't call any more callbacks (only valid if first exclusive subscriber)
#define INPUT_FLAG_EXCLUSIVE 8
//Call on move (joystick/mouse)
#define INPUT_FLAG_MOVEMENT 16


//inputMouse/Stick event contains motion
#define INPUT_EVENT_TYPE_MOTION 0
//inputMouse/Stick event contains buttonpress
#define INPUT_EVENT_TYPE_BUTTON 1

typedef struct {
  SDL_keysym sym;
  int timeDown; //Only set if subscribed INPUT_KEYHOLD
} inputKey;

typedef struct {
  int_fast8_t type;
  SDL_JoyAxisEvent    motion;
  SDL_JoyButtonEvent  button;
} inputStick;

typedef struct {
  int_fast8_t type;
  SDL_MouseMotionEvent motion;
  SDL_MouseButtonEvent button;
} inputMouse;

typedef struct {
  inputKey* key;      //Eventdata for keyboard
  inputMouse* mouse;  //For mouse
  inputStick* stick;  //For joystick
} inputEvent;

typedef void (*inputCallback)(inputEvent*);


/*******************************************************************************
  GUI system structures and definitions
*******************************************************************************/
//A motif style windo
#define GUI_TYPE_WINDOW 0
//A static text label
#define GUI_TYPE_LABEL 1
//A button with text and a callback
#define GUI_TYPE_BUTTON 2
//An image (sprite)
#define GUI_TYPE_IMAGE 3
//A scrollable textbox.
#define GUI_TYPE_TEXTBOX 4

#define BTN_HIDECLOSE (void*)0
#define BTN_SHOWCLOSE (void*)1
#define GUI_WIN_HIDECLOSE BTN_HIDECLOSE
#define GUI_WIN_SHOWCLOSE BTN_SHOWCLOSE

#define GUI_FADE_IN 1
#define GUI_FADE_OUT 2
#define GUI_FADE_DONE 3
#define GUI_FADE_QUERY 4
#define GUI_POS_CENTER -16384

#define GUI_NO_CONTEXT (void*)0


typedef void (*guiCallback)(void*);

typedef struct {
  vec2 pos;
  vec2 _size;
  GLfloat colBg[4]; //Initialized to a lovely grey semi transperant.
  GLfloat colBorder[4]; //And a sligthly lighter grey
  bool _draw;     //Draw the window, or only it's elements
  bool showTitle; //Show title line?
  bool showClose; //Show close button (only if title line is shown)
  bool movable;   //Allow dragging of window.
  int font,fontPos;
  char* title;
  guiCallback callbackOnClose; //Only called if callback != 0
  void* _packed;   //Parent window if any (we'll cast it later)
  listItem* elements; //Elements in this window.
} guiWindow_s;

typedef struct {
  vec2 pos;
  int font,fontPos;
  char* txt;
  guiCallback callback;
  void* callbackData;
} guiLabel_s;

typedef struct {
  vec2 pos;
  vec2 size;
  GLfloat colBg[4];
  GLfloat colBorder[4];
  int font,fontPos;
  char* txt;
  guiCallback callback;
  void* callbackData;
} guiButton_s;

typedef struct {
  vec2 pos;
  sprite_s* sprite;
  guiCallback callback;
  void* callbackData;
} guiImage_s;

typedef struct {
  vec2 pos;
  vec2 size;
  int font,fontPos;
  char** _lines;     //Array of lines, inputstring is split by \n
  int    _numLines;  //Number of lines in array
  int    _scrollLines;//Line currently starting the scrollbuffer
} guiTextBox_s;

typedef struct {
  int type;     //Type of object
  GLubyte idCol[3]; //Really easy way to detect mouse hit.
  void* data;   //Pointer to the object
} guiElement_s;




/*******************************************************************************
  Font definitions
*******************************************************************************/
//"normal" standard gui font
#define FONT_S_NAME "TakaoExGothic.ttf"
#define FONT_M_NAME "256BYTES.TTF"
#define FONT_L_NAME "256BYTES.TTF"
#define FONT_SMALL 0 //Font Normal Small
#define FONT_MEDIUM 1 //Font Normal Medium
#define FONT_LARGE 2 //Font Normal Large

#define FONT_SYS_NAME "8x13-iso8859-1.fon"
//"system" font
#define FONT_SYS 3 //Font Normal Small

#define NUM_FONTS 4

#define TXT_CENTER 0
#define TXT_LEFT 1
#define TXT_RIGHT 2

/*******************************************************************************
  GFX engine structure
*******************************************************************************/
typedef struct {
  GLuint fbo, tex;
  GLint w,h; //Viewport size in pixels
  GLint tw,th; //Texture size in pixels
  GLfloat s,t; //Texture coords in unitspace
} renderTex_t; //Texture used with FBO to render to texture.


#endif // TYPES_H_INCLUDED
