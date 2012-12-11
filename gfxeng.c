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

#include "gl.h"


#include "types.h"
#include "data.h"
#include <math.h>
#include "gfxeng.h"
#include "gltxt.h"

#include "camera.h"
#include "input.h"

#include "console.h"

#include "screenshot.h"
void initEnvMap();

static int engShowTestBox=0;
static int engTestBoxId=0;

void gfxEngInit()
{
  glewExperimental = GL_TRUE ;
  glewInit();
  initGL();
  initEnvMap(GL_TEXTURE1, "/data/maps/smoke/");
  initEnvMap(GL_TEXTURE2, "/data/maps/scenery1/");
  gltxtInit(eoSetting()->res.y);
  //Init the console
  consoleInit();

  //Hook stuffs and functions
  eoFuncAdd( inputShowBinds, NULL, "binds" );
  eoFuncAdd( cameraFreeLook, NULL, "camfree" );
  eoFuncAdd( &cameraBeginRecord, NULL, "camstartrecord" );
  eoFuncAdd( &cameraEndRecord, NULL, "camstoprecord" );
  eoFuncAdd( &camConPlayRec, NULL, "camplayrecord" );
  eoFuncAdd( cameraSetSens, NULL, "camsensitivity" );
  eoFuncAdd( cameraGrabCursor, NULL, "camgrabcursor" );
  eoFuncAdd( cameraLockLook, NULL, "camlocktarget" );

  eoVarAdd(CON_TYPE_VEC3,0, &camGet()->pos, "campos");
  eoVarAdd(CON_TYPE_VEC3,0, &camGet()->target, "camlook");
  eoVarAdd(CON_TYPE_FLOAT,0, &camGet()->zoom, "camzoom");
  eoVarAdd(CON_TYPE_FLOAT,0, &camGet()->zNear, "camznear");


  eoVarAdd(CON_TYPE_INT, 0, &engShowTestBox, "testbox");

  eoFuncAdd( screenShotConsole, NULL, "screenshot" );
  eoInpAddHook( INPUT_EVENT_KEY, INPUT_FLAG_DOWN, SDLK_F12, screenShotInput );

  //Test grid
  const GLfloat width=20, height=10,length=30;
  engTestBoxId= glGenLists( 1 );
  glNewList( engTestBoxId, GL_COMPILE );
    glDisable( GL_LIGHTING );
    glLineWidth(1);
    glEnable( GL_COLOR_MATERIAL );
    glDisable( GL_TEXTURE_2D );
    glLineWidth(1.5);
    glBegin( GL_LINES );
      //X
      glColor4f(1,0,0,1);
      glVertex3f(-width,height,length);
      glVertex3f(width,height,length);

      glVertex3f(-width,-height,length);
      glVertex3f(width,-height,length);

      glVertex3f(-width,height,-length);
      glVertex3f(width,height,-length);

      glVertex3f(-width,-height,-length);
      glVertex3f(width,-height,-length);

      //X cross1
      glVertex3f(-width,height,length);
      glVertex3f(width,height,-length);

      glVertex3f(width,height,length);
      glVertex3f(-width,height,-length);

      //X Cross2
      glVertex3f(-width,-height,length);
      glVertex3f(width,-height,-length);

      glVertex3f(width,-height,length);
      glVertex3f(-width,-height,-length);



      //Y
      glColor4f(0,1,0,1);
      glVertex3f(width,-height,length);
      glVertex3f(width,height,length);

      glVertex3f(-width,-height,length);
      glVertex3f(-width,height,length);

      glVertex3f(width,-height,-length);
      glVertex3f(width,height,-length);

      glVertex3f(-width,-height,-length);
      glVertex3f(-width,height,-length);

      //Y cross 1
      glVertex3f(width,-height,length);
      glVertex3f(width,height,-length);

      glVertex3f(width,height,length);
      glVertex3f(width,-height,-length);
      //Y cross 2
      glVertex3f(-width,-height,length);
      glVertex3f(-width,height,-length);

      glVertex3f(-width,height,length);
      glVertex3f(-width,-height,-length);



      //Z
      glColor4f(0,0,1,1);
      glVertex3f(width,height,-length);
      glVertex3f(width,height,length);

      glVertex3f(-width,height,-length);
      glVertex3f(-width,height,length);

      glVertex3f(width,-height,-length);
      glVertex3f(width,-height,length);

      glVertex3f(-width,-height,-length);
      glVertex3f(-width,-height,length);

      //Z cross 1
      glVertex3f(width,height,-length);
      glVertex3f(-width,-height,-length);

      glVertex3f(width,-height,-length);
      glVertex3f(-width,height,-length);

      //Z cross2
      glVertex3f(width,height,length);
      glVertex3f(-width,-height,length);

      glVertex3f(width,-height,length);
      glVertex3f(-width,height,length);

    glEnd();

    glDisable( GL_COLOR_MATERIAL );
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_LIGHTING );

  glEndList();


}

settings_t program_settings;
settings_t* eoSetting() { return(&program_settings); }


void initGL()
{
  glViewport(0, 0, (GLint) eoSetting()->res.x, (GLint)eoSetting()->res.y);


  glClearColor(0,0,0,1);

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


  GLfloat ambientLight[] = { 0.1f, 0.1f, 0.1f, 1.0f };
  GLfloat diffuseLight[] = { 1, 1, 1, 1.0f };
  GLfloat specularLight[] = { 1, 1, 1, 1.0f };

  // Assign created components to GL_LIGHT0
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);


  GLfloat global_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);


//  glClearAccum(0.0, 0.0, 0.0, 1.0);

//  glClear(GL_ACCUM_BUFFER_BIT);

  glEnable(GL_COLOR_MATERIAL);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable( GL_CULL_FACE );
  glEnable(GL_DEPTH_TEST);

  GLfloat pos[] = { 0,60,0,1 }; //Last pos: 0 = dir, 1=omni
  glLightfv( GL_LIGHT0, GL_POSITION, pos );


}


GLenum _engSdlSurf2GlFormat( SDL_Surface* surf )
{
  if( surf->format->BitsPerPixel==24)
  {
    return(GL_RGB);
  } else if( surf->format->BitsPerPixel==32)
  {
    return(GL_RGBA);
  } else {
    return(GL_RGB4);
  }
}


void initEnvMap(GLenum textarget, const char* dir)
{
    // Try and make a enviroment map
  SDL_Surface *tpx, *tnx, *tpy, *tny, *tpz, *tnz;

  tpx = IMG_Load( Data(dir,"px.png") );
  tnx = IMG_Load( Data(dir,"nx.png") );

  tpy = IMG_Load( Data(dir,"py.png") );
  tny = IMG_Load( Data(dir,"ny.png") );

  tpz = IMG_Load( Data(dir,"pz.png") );
  tnz = IMG_Load( Data(dir,"nz.png") );

  if( !tpx || !tnx || !tpy || !tny || !tpz || !tnz )
  {
    eoPrint("^2envMap Error: Couldn't load one of the textures in '%s':",Data(dir,"") );
    eoPrint("^2              Required files are px.png nz.png py.png ny.png pz.png nz.png");
  }

  GLuint cubeMapTexId;
  GLuint width = tpx->w;
  GLuint height= tpx->h;

  glGenTextures(1, &cubeMapTexId);

  glActiveTexture( textarget );

  glEnable(GL_TEXTURE_CUBE_MAP);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexId);

  glColor4f(0,1,0,0.3);

  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD_SIGNED );

  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  //Define all 6 faces
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, width, height, 0, _engSdlSurf2GlFormat(tpx), GL_UNSIGNED_BYTE, tpx->pixels);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, width, height, 0, _engSdlSurf2GlFormat(tnx), GL_UNSIGNED_BYTE, tnx->pixels);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, width, height, 0, _engSdlSurf2GlFormat(tpy), GL_UNSIGNED_BYTE, tpy->pixels);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, width, height, 0, _engSdlSurf2GlFormat(tny), GL_UNSIGNED_BYTE, tny->pixels);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, width, height, 0, _engSdlSurf2GlFormat(tpz), GL_UNSIGNED_BYTE, tpz->pixels);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, width, height, 0, _engSdlSurf2GlFormat(tnz), GL_UNSIGNED_BYTE, tnz->pixels);

  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
  //
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_GEN_R);

  SDL_FreeSurface(tpx);
  SDL_FreeSurface(tnx);
  SDL_FreeSurface(tpy);
  SDL_FreeSurface(tny);
  SDL_FreeSurface(tpz);
  SDL_FreeSurface(tnz);

  glDisable( GL_TEXTURE_CUBE_MAP  );

  glActiveTexture( GL_TEXTURE0 );
  glDisable(GL_TEXTURE_CUBE_MAP);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_TEXTURE_GEN_R);
}


GLuint eoGfxTexFromSdlSurf( SDL_Surface* tex )
{
  GLuint texture;
  glEnable(GL_TEXTURE_2D);
  glGenTextures( 1, &texture );
  glBindTexture( GL_TEXTURE_2D, texture );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );


  GLenum pixelPacking;
  if( tex->format->BitsPerPixel==24)
  {
    pixelPacking=GL_RGB;
  } else if( tex->format->BitsPerPixel==32)
  {
    pixelPacking=GL_RGBA;
  } else {
    pixelPacking = GL_RGB4;
  }
  //Upload to ogl
  glTexImage2D( GL_TEXTURE_2D, 0, tex->format->BytesPerPixel, tex->w, tex->h, 0, pixelPacking, GL_UNSIGNED_BYTE, tex->pixels );

  //For mipmapped textures
  /*gluBuild2DMipmaps( GL_TEXTURE_2D, 3, tex->w, tex->h, pixelPacking, GL_UNSIGNED_BYTE, tex->pixels );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );*/

  return( texture );
}

GLuint eoGfxLoadTex( const char* fileName )
{
  GLuint texName=0;
  SDL_Surface* tex = IMG_Load( fileName );

  if(tex)
    texName=eoGfxTexFromSdlSurf(tex);

  SDL_FreeSurface(tex);
  return(texName);
}

void engFreeTex( GLuint texName )
{
  glDeleteTextures( 1, &texName );
}

void engRender()
{
  camBegin();

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable( GL_CULL_FACE );
  glEnable(GL_DEPTH_TEST);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);


  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if( engShowTestBox )
    glCallList( engTestBoxId );
}


void eoGfxFboRenderBegin( renderTex_t* rt )
{
  glPopAttrib();
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rt->fbo);
  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(0,0,rt->w,rt->h);
  glClear( GL_COLOR_BUFFER_BIT );
}

void eoGfxFboRenderEnd()
{
  glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
}


renderTex_t* eoGfxFboCreate(int width, int height)
{
  renderTex_t* rt = malloc( sizeof(renderTex_t) );
  rt->w = width;
  rt->h = height;
  //Get best POT texture size
  rt->tw = eoBestPOT( width );
  rt->th = eoBestPOT( height);
  //Calculate texcoords
  rt->s = (GLfloat)rt->w/(GLfloat)rt->tw;
  rt->t = (GLfloat)rt->h/(GLfloat)rt->th;
  glGenFramebuffersEXT( 1, &rt->fbo );
  glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rt->fbo );
  glGenTextures( 1, &rt->tex );
  glBindTexture( GL_TEXTURE_2D, rt->tex );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, rt->tw,rt->th, 0,GL_RGBA, GL_UNSIGNED_BYTE, NULL );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->tex, 0 );


  if( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT )
    eoPrint("eoGfxFboCreate Error: Framebuffer incomplete:%i",glCheckFramebufferStatus(GL_FRAMEBUFFER));
  else
    eoPrint("eoGfxFboCreate Created fbo %i texture %i (%ix%i) Viewport %ix%i Texcoords: %f, %f", rt->fbo,rt->tex,rt->tw, rt->th, rt->w, rt->h, rt->s, rt->t);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  return(rt);
}

void eoGfxFboDel( renderTex_t* rt )
{
  glDeleteTextures(1, &rt->tex);
  glDeleteFramebuffers(1, &rt->fbo);
}

int eoBestPOT( int i )
{
  return (pow(2, ceil( log2( i ) ) ) );
}

void eoGfxBillboardBegin()
{
	GLfloat modelview[16];
	int i,j;

	// save the current modelview matrix
	glPushMatrix();

	// get the current modelview matrix
	glGetFloatv(GL_MODELVIEW_MATRIX , modelview);

	// undo all rotations
	// beware all scaling is lost as well
	for( i=0; i<3; i++ )
	    for( j=0; j<3; j++ ) {
		if ( i==j )
		    modelview[i*4+j] = 1.0;
		else
		    modelview[i*4+j] = 0.0;
	    }

	// set the modelview with no rotations
	glLoadMatrixf(modelview);

}

void eoGfxBillBoardEnd()
{
	// restore the previously
	// stored modelview matrix
	glPopMatrix();

}

GLfloat eoRandFloat( GLfloat max )
{
  if( max > 0 )
  {
    max *= 10000.0;
    GLfloat d = (GLfloat)(rand()%(int)max);
    return( d/10000.0 );
  }
  return(0);
}

GLfloat eoVec3Len( vec3 v )
{
  return( (GLfloat)sqrt( v.x*v.x +  v.y*v.y + v.z*v.z ) );
}

vec3 eoVec3Normalize( vec3 v )
{
  vec3 nv;
  GLfloat len = eoVec3Len( v );
  nv.x = v.x / len;
  nv.y = v.y / len;
  nv.z = v.z / len;
  return(nv);
}

vec3 eoVec3Scale( vec3 v, GLfloat len )
{
  vec3 nv;
  nv.x = v.x*len;
  nv.y = v.y*len;
  nv.z = v.z*len;
  return(nv);
}

vec3 eoVec3FromPoints( vec3 pa, vec3 pb ) //Get an unnormalized directional vector describing the length and direction from pointA to pointB
{
  pb.x -= pa.x;
  pb.y -= pa.y;
  pb.z -= pa.z;
  return( pb );
}

//Return a normalvector from rotation around x and y
vec3 eoVec3FromAngle( GLfloat radx, GLfloat rady )
{
    vec3 v;
    GLfloat yscale  = sin(rady);

    v.x = cos( radx)*yscale;
    v.y = cos(rady);
    v.z = sin( radx )*yscale;

    return(v);
}

vec3 eoVec3Add( vec3 a, vec3 b)
{
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  return(a);
}

vec2 engRadFromPoints( vec3 a, vec3 b )
{
  vec2 p;
  //Rotation around X axis
  p.y = atan2( b.x-a.x, b.y-a.y );

  //Rotation around Y axis
  p.x = atan2( b.z-a.z, sqrt( ((b.x-a.x)*(b.x-a.x))+((b.y-a.y)*(b.y-a.y)) ) );
  return(p);
}
