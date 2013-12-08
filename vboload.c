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

#include "vboload.h"
#include "console.h"
#include "strings.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "gfxeng.h"
#include "data.h"

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <stdio.h>

#include "types.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

//Since names are found from mtlib and needed when reading the obj. This is allocated by readMtlLib, and freed by eoModelLoad
static matProps* materials;

typedef struct { int vi[3], ti[3], ni[3]; int mtlIdx; } objFace;

#pragma pack(push, 1)
typedef struct {
  GLfloat x;
  GLfloat y;
  GLfloat z;
  GLfloat nx;
  GLfloat ny;
  GLfloat nz;
  GLfloat u;
  GLfloat v;
} vData;
#pragma pack(pop)

//Used for sorting according to material
int compMat( const void* facea, const void* faceb )
{
  return ( ((objFace*)facea)->mtlIdx - ((objFace*)faceb)->mtlIdx );
}

//returns index of material in model's materialarray, with name supplied, -1 if not.
int getMaterialIndex(vboModel* model, char* name)
{
  int i;
  for(i=0;i < model->matCount; i++)
  {
    if( strcmp( model->materials[i].name, name ) == 0)
    {
      return(i);
    }
  }
  return(-1);
}


//Reads a material lib file and allocates/fills model->materials model->matCount
void readMtlLib( vboModel* model, const char* dir,const char* fileName )
{
  int i;
  char line[512];
  char bufa[512];
  char bufb[512];
  char bufc[512];

  FILE* file = fopen( Data(dir, fileName), "r" );
  if(!file)
  {
    eoPrint("Error: Couldn't open '%s' for reading.", Data(dir,fileName) );
    return;
  }

  //Count number of materials.
  model->matCount=0;
  while(fgets(line, 511, file))
  {
    if( strlen(line)>6)
    {
      line[6]=0;
      if( strcmp(line, "newmtl") == 0)
        model->matCount++;
    }

  }


  eoPrint("  Found %i materials in file.", model->matCount);

  materials = malloc( sizeof( matProps )* model->matCount ); //Used to collect material properties for compiling the list.
  //Create displaylists for materials.
  int baseListId = glGenLists( model->matCount );

  //Create material array for model
  model->materials = malloc( sizeof(modelMaterial)*model->matCount );
  model->textureName = malloc( 128 );
  strcpy( model->textureName, "Error: No texture defined!" );
  rewind(file);

  int cMtl=-1; //Current index, bumped by newmtl

  char restore=0;

  while(fgets(line, 511, file))
  {
    stripNewLine(line);
    if( strlen(line)>6)
    {
      restore=line[6];
      line[6]=0;

      if( strcmp(line, "newmtl") == 0)
      {
        line[6]=' ';
        cMtl++;
        splitVals(' ', line, bufa, bufb);

        model->materials[cMtl].name = malloc(sizeof(char)*strlen(bufb)+1);
        //materials[cMtl].name = malloc(sizeof(char)*strlen(bufb)+1);
        strcpy( model->materials[cMtl].name, bufb );
        //strcpy( materials[cMtl].name, bufb );
        eoPrint("  Material[%i].name: '%s'", cMtl,model->materials[cMtl].name);

        //set alpha on specular and diffuce lightning.
        materials[cMtl].ambDifCol[3] = 1;
        materials[cMtl].specCol[3] = 1.0;
        //Set no cubemap
        materials[cMtl].cubemap = 0;
      }

      //Let's break the spec and only support the first texture found, since switching textures for model gemoetry is not cool anyway.
      if( strcmp(line, "map_Kd") == 0)
      {
        //Only add texture if there is none
        if( !model->texture )
        {
          line[6]=' ';
          splitVals(' ', line, bufa, bufb);

          free( model->textureName );
          model->textureName = malloc( sizeof(char)*strlen(Data(dir,bufb) )+1);
          strcpy( model->textureName, Data(dir,bufb) );
          model->texture = eoGfxLoadTex( model->textureName );
          if(!model->texture)
          {
            eoPrint("  Error: Couldn't load texture: %s", SDL_GetError());
          } else {
            eoPrint("  Using map_Kd '%s' as texture for whole model.", bufb);
          }

        }
      }

      line[6]=restore; //what a hack.
    } //Check new materials
    //NO ELSE!

    if( strlen(line) > 5 )
    {
      restore=line[4];
      line[4]=0;
      if( strcmp( "refl", line) == 0 )
      {
        line[4]=' ';
        sprintf(bufa, "%s", line+4);
        int map = atoi(bufa);
        eoPrint("  Envmap '%i'", map);
        switch( map )
        {
          case 1:
            materials[cMtl].cubemap = GL_TEXTURE1;
          break;
          case 2:
            materials[cMtl].cubemap = GL_TEXTURE2;
          break;
          case 3:
            materials[cMtl].cubemap = GL_TEXTURE3;
          break;
          default:
            eoPrint("Invalid cubemap specified.");
          break;
        }
      }

      line[4]=restore;
    }

    if( strlen(line) > 2)
    {
      if( line[0] == 'd' && line[1] == ' ' )
      {
        sprintf(bufa, "%s", line+2);
        materials[cMtl].ambDifCol[3] = (float)atof( bufa );
      }
    }

    if( strlen(line)>3)
    {
      //Specular exponent / shininess
      if(line[0] == 'N' && line[1] == 's' && line[2] == ' ')
      {
        materials[cMtl].shininess = (0.128*(float)atof( line+3) );
      } else
      //Ambient reflection
      if( line[0] == 'K' && line[1] == 'a' && line[2] == ' ' )
      {
        //Ignore, since blender don't seem to use it anyway
      } else
      //Diffuse reflection, also set ambint since opengl likes that
      if( line[0] == 'K' && line[1] == 'd' && line[2] == ' ' )
      {
        splitVals(' ', line+3, bufa, bufb);
        strcpy(line, bufb);
        splitVals(' ', line, bufb, bufc);

        materials[cMtl].ambDifCol[0] = (float)atof( bufa );
        materials[cMtl].ambDifCol[1] = (float)atof( bufb );
        materials[cMtl].ambDifCol[2] = (float)atof( bufc );
      } else
      //Specular reflection
      if( line[0] == 'K' && line[1] == 's' && line[2] == ' ' )
      {
        splitVals(' ', line+3, bufa, bufb);
        strcpy(line, bufb);
        splitVals(' ', line, bufb, bufc);

        materials[cMtl].specCol[0] = (float)atof( bufa );
        materials[cMtl].specCol[1] = (float)atof( bufb );
        materials[cMtl].specCol[2] = (float)atof( bufc );
      }

    }

  }

  for(i=0; i < model->matCount; i++)
  {
    int listId = baseListId + i;
    eoPrint("  Compiling list %i for material[%i] '%s'", listId, i, model->materials[i].name);
    //Fetch material (Yes, I'm that lazy)
    matProps* mat = &materials[i];

    model->materials[i].matDL = listId;
    model->materials[i].cubemap = mat->cubemap;
    glNewList( listId, GL_COMPILE );
      glDisable(GL_COLOR_MATERIAL);

      //Set material props
      glMaterialf( GL_FRONT, GL_SHININESS, mat->shininess );
      glMaterialfv( GL_FRONT, GL_SPECULAR, mat->specCol );

      glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat->ambDifCol );

      //Do we want envmapping on this material?
      if( mat->cubemap != 0 )
      {
          glActiveTexture( mat->cubemap);
          glEnable( GL_TEXTURE_CUBE_MAP );
      }
    glEndList();

    eoPrint("   Material[%i] spec: %f, %f, %f %f", i, materials[i].specCol[0],materials[i].specCol[1], materials[i].specCol[2], materials[i].specCol[3]);
    eoPrint("   Material[%i] shin: %f", i, materials[i].shininess );
    eoPrint("   Material[%i] ambDif %f, %f, %f", i, materials[i].ambDifCol[0],materials[i].ambDifCol[1],materials[i].ambDifCol[2]);
    eoPrint("   Material[%i] alpha (ambDif[3]) %f", i, materials[i].ambDifCol[3]);
    eoPrint("   Material[%i] Kd_map: %s",i, model->textureName );
  }

}




//Loads an obj file and returns the initialized model. Caller must //free. Return 0 on err
vboModel* eoModelLoad( const char* dir, const char* fileName )
{
  eoPrint("eoModelLoad('%s','%s');",dir, fileName);
  int i;
  char line[512];
  char bufa[512];
  char bufb[512];
  char bufc[512];

  //In case of more materials, we ignore the rest.
  materials=0;

  //Try to open file for reading.
  FILE* file = fopen( Data(dir, fileName), "r" );
  if(!file)
  {
    eoPrint("<1>Error: Couldn't open '%s' for reading.", Data(dir, fileName));
    return(0);
  }

  //Count how much datastorage we need
  int numVerticies=0, numNormals=0, numTexCoords=0, numFaces=0;
  while(fgets(line, 511, file))
  {
    stripNewLine(line);
    //Check type
    if( strlen(line)>5)
    {
     // eoPrint("Line '%s' is %i bytes.", line, strlen(line));
      //Vertex has a v and a space
      if( line[0] == 'v' && line[1] == ' ' )
      {
        numVerticies++;
      } else
      //Vertex normals have v n and space
      if( line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
      {
        numNormals++;
      } else
      //Texture coords have vt
      if( line[0] == 'v' && line[1] == 't' && line[2] == ' ')
      {
        numTexCoords++;
      } else
      //Faces have f and a space
      if( line[0] == 'f' && line[1] == ' ' )
      {
        numFaces++;
      } //Face
    } //long enough to be meaningfull
  }

  eoPrint("  %i verticies, %i normals, %i texcoords and %i faces..", numVerticies, numNormals, numTexCoords, numFaces);

  //Temporary arrays to store obj data
  int vertexIndex=0, normalIndex=0, texCoordIndex=0, faceIndex=0;
  vec3* vertexData = malloc( sizeof(vec3)*numVerticies );
  vec3* normalData = malloc( sizeof(vec3)*numNormals );
  vec2* texCoordData = malloc( sizeof(vec2)*numTexCoords );
  objFace* faceData = malloc( sizeof(objFace)*numFaces );
  //Our vboModel
  vboModel* model = malloc(sizeof(vboModel));
  model->texture = 0; //So we can check if there's any texture.
  model->drawType = GL_TRIANGLES; //Default draw mode for all objects


  //Rewind file ptr
  rewind(file);

  //Current materal
  int currentMtl=-1;

  //We've allocated the memory so we're ready to fill the arrays.
  while(fgets(line, 511, file))
  {
    stripNewLine(line);

    //Check type
    if( strlen(line)>5)
    {
     // eoPrint("Line '%s' is %i bytes.", line, strlen(line));
      //Vertex has a v and a space
      if( line[0] == 'v' && line[1] == ' ' )
      {

        //Parse vertex
          splitVals(' ', line+( (line[2]==' ')?3:2), bufa, bufb);

        strcpy( line, bufb );
        splitVals(' ', line, bufb, bufc);
        vertexData[vertexIndex].x = (float)atof( bufa );
        vertexData[vertexIndex].y = (float)atof( bufb );
        vertexData[vertexIndex].z = (float)atof( bufc );
//        eoPrint("BufA '%s'", bufa);
//        eoPrint("BufB '%s'", bufb);
//        eoPrint("BufC '%s'", bufc);

        vertexIndex ++;
      } else
      //Vertex normals have v n and space
      if( line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
      {
        //Parse vertex normal
        splitVals(' ', line+3*sizeof(char), bufa, bufb);
        strcpy( line, bufb );
        splitVals(' ', line, bufb, bufc);

        normalData[normalIndex].x = (float)atof( bufa );
        normalData[normalIndex].y = (float)atof( bufb );
        normalData[normalIndex].z = (float)atof( bufc );
        normalIndex++;

      } else
      //Texture coords have vt
      if( line[0] == 'v' && line[1] == 't' && line[2] == ' ')
      {
        //Parse texture coords
        splitVals(' ', line+3*sizeof(char), bufa, bufb);

        texCoordData[texCoordIndex].x = (float)atof( bufa );
        texCoordData[texCoordIndex].y = (float)atof( bufb );
        texCoordIndex ++;

      } else
      //Faces have f and a space
      if( line[0] == 'f' && line[1] == ' ' )
      {
        //parse face ( vi/ti/ti vi/ti/ni vi/ti/ni ) because we only support triangulated faces.
        //Explode to sets of vi/ti/ni strings stored in t[0] 1 2
        char** t = explode( ' ', line+2, 3 );

        for(i=0; i<3; i++)
        {
          if(t[i])
          {
            char** f = explode('/', t[i], 3);

            faceData[faceIndex].vi[i] = atoi( f[0] );
            faceData[faceIndex].ti[i] = atoi( f[1] );
            faceData[faceIndex].ni[i] = atoi( f[2] );
            free( f[0] );
            free( f[1] );
            free( f[2] );
            free(f);
          } else {
            eoPrint("T[%i]: is empty.",i);
          }
        }

        //Put on material
        if(currentMtl!=-1)
        {
          faceData[faceIndex].mtlIdx = currentMtl;
        } else {
          faceData[faceIndex].mtlIdx = 0;
          //eoPrint("Error: face %i have no material.", faceIndex);
        }
        faceIndex++;
        free(t[0]);
        free(t[1]);
        free(t[2]);
        free(t);
      } else //Material lib
      if( line[0] == 'm' && line[1] == 't' && line[2] == 'l' )
      {
        splitVals(' ', line, bufa, bufb);
        if( !materials )
        {
          eoPrint("  Using material lib: '%s' for whole file.", bufb);
          model->matCount=-1;
          readMtlLib( model, dir, bufb );
        } else {
          eoPrint("ERROR: Encountered more material libs Only one is supported, ignoring.");
        }
      } else //Material usage usem is close enough to usemtl
      if( line[0] == 'u' && line[1] == 's' && line[2] == 'e' && line[3] == 'm' )
      {
        splitVals(' ', line, bufa, bufb);

        currentMtl = getMaterialIndex(model, bufb);
        if(currentMtl==-1)
        {
          eoPrint("Error: Material:'%s' not found!",bufb);
        }
      } else
      //smooth
      if( line[0] == 's' && line[1] == ' ' )
      {
        if(line[2] == 'o' || line[2] == 'O')
         eoPrint("Fixme: Flat shade model not implemented.");
      }
    }
  }
  fclose( file );
  //3 glfloats pr face
  model->tris = numFaces*3; //A face is always a triangle, which have 3 verts
  model->vertexDataSize = model->tris*sizeof(vData);

  //Sort faces after material used
  qsort( faceData, numFaces, sizeof(objFace), compMat );

  vData* data = malloc( model->vertexDataSize );
  if( !data  )
  {
    eoPrint("Error: Couldn't allocate %i KiB.", model->vertexDataSize);
  }

  int index=0;
  int count=0; //Number of triangles in each material group
  //Interleave array with data from vertexData, normalData, texCoordData and faceData.
  int cMtl=-1;

  //To figure out size of hitbox
  GLfloat xmin=0,xmax=0,zmin=0,zmax=0,ymin=0,ymax=0;

  for(i=0; i<numFaces; i++)
  {
    //Set start index for material.
    if( cMtl != faceData[i].mtlIdx )
    {
      cMtl = faceData[i].mtlIdx;
      model->materials[cMtl].start = index;
      count=0;
    }

    int ii;
    for(ii=0; ii<3; ii++)
    {
      data[index].x = vertexData[ (faceData[i].vi[ii]-1) ].x;
      data[index].y = vertexData[ (faceData[i].vi[ii]-1) ].y;
      data[index].z = vertexData[ (faceData[i].vi[ii]-1) ].z;

      //Find biggest deviance for hitbox
      if(data[index].x > xmax)
        xmax = data[index].x;
      else if( data[index].x < xmin )
        xmin = data[index].x;

      if(data[index].y > ymax)
        ymax = data[index].y;
      else if( data[index].y < ymin )
        ymin = data[index].y;

      if(data[index].z > zmax)
        zmax = data[index].z;
      else if( data[index].z < zmin )
        zmin = data[index].z;




      data[index].nx = normalData[ (faceData[i].ni[ii]-1) ].x;
      data[index].ny = normalData[ (faceData[i].ni[ii]-1) ].y;
      data[index].nz = normalData[ (faceData[i].ni[ii]-1) ].z;

      //Normalize normalvector
      GLfloat len = (GLfloat)sqrt( data[index].nx*data[index].nx +  data[index].ny*data[index].ny + data[index].nz*data[index].nz );
      data[index].nx /= len;
      data[index].ny /= len;
      data[index].nz /= len;

      data[index].u = texCoordData[ (faceData[i].ti[ii]-1) ].x;
      data[index].v = texCoordData[ (faceData[i].ti[ii]-1) ].y*-1; //Flip
      index++;
      count++;
    }

    //This way we always set stop index correctly.
    model->materials[cMtl].count = count;
    model->tris = index; //For clay render
  }

  //Calculate size
  model->size.x = (fabs(xmin)+fabs(xmax))/2.0;
  model->size.y = (fabs(ymin)+fabs(ymax))/2.0;
  model->size.z = (fabs(zmin)+fabs(zmax))/2.0;




  //Generate VBO
  glGenBuffers(1, &model->bufferName);
  //Upload to opengl.
  glBindBuffer(GL_ARRAY_BUFFER, model->bufferName);
  glBufferData(GL_ARRAY_BUFFER, model->vertexDataSize, data, GL_STATIC_DRAW );

  //
  glBindBuffer(GL_ARRAY_BUFFER, 0);


  //Free our data, ogl got what it wanted.
  free(data);
  free(vertexData);
  free(normalData);
  free(texCoordData);
  free(faceData);


  model->name = malloc( strlen(fileName)+1 );
  model->dir = malloc( strlen( dir )+1 );
  strcpy( model->name, fileName );
  strcpy( model->dir, dir );
   //Loaded model overview for debug
  eoPrint("  Model Overview (%s):", model->name);
  eoPrint("   VBO buffer name: %i", model->bufferName);
  eoPrint("   %i triangles.", model->tris );
  eoPrint("   %i materials.", model->matCount);
  eoPrint("   %i KiB.", model->vertexDataSize/1024);

  //Free material struct
  memset( materials, 0, sizeof( matProps )* model->matCount );
  free(materials);

  return(model);
}

//
void drawModel( vboModel* model, int_fast8_t fullBright )
{
  int cMat=0;
  //Set base texture for model
  glEnable( GL_TEXTURE_2D );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture(GL_TEXTURE_2D, model->texture);

  //Bind buffers
  glBindBuffer(GL_ARRAY_BUFFER, model->bufferName);

  //Verties first in array
  glVertexPointer( 3, GL_FLOAT,sizeof(vData)  , 0 );
  //Normals position
  glNormalPointer(GL_FLOAT, sizeof(vData), (GLvoid*)(sizeof(GLfloat)*3)  );
  //Texcoords position
  glTexCoordPointer( 2, GL_FLOAT, sizeof(vData), (GLvoid*)(sizeof(GLfloat)*6) );
  //Lights enabled?
  if( fullBright )
  {
    glDisable( GL_LIGHTING );
    glColor4f(1,1,1,1);
  } else {
    glEnable( GL_LIGHTING );
  }

  //Draw geometry.
  do
  {
    //Be lazy.
    modelMaterial* mat = &model->materials[cMat];
    //Apply material
    glCallList( mat->matDL );


    //Draw verts
    glDrawArrays( model->drawType, mat->start, mat->count);

    //Disable cubemap if any
    if( mat->cubemap != 0 )
    {
        glActiveTexture( mat->cubemap );
        glDisable( GL_TEXTURE_CUBE_MAP );
    }


    //Go to next material
    cMat++;
  } while( cMat< model->matCount );

  if( fullBright )
    glEnable( GL_LIGHTING );

  glActiveTexture( GL_TEXTURE0 );
}

void drawClayModel( vboModel* model, GLubyte c[4], int_fast8_t allWhite )
{

  int cMat=0;
    if(allWhite)
    {
      glDisable( GL_LIGHTING );
    }

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable( GL_TEXTURE_2D );
    //Bind buffers
    glBindBuffer(GL_ARRAY_BUFFER, model->bufferName);

    //Verties first in array
    glVertexPointer( 3, GL_FLOAT,sizeof(vData)  , 0 );
    //Normals position
    glNormalPointer(GL_FLOAT, sizeof(vData), (GLvoid*)(sizeof(GLfloat)*3)  );
    //Texcoords position
    glTexCoordPointer( 2, GL_FLOAT, sizeof(vData), (GLvoid*)(sizeof(GLfloat)*6) );

    //set Clay color
    glColor4ubv( c );

    //Draw geometry.
    do
    {
      modelMaterial* mat = &model->materials[cMat];
      if( !allWhite )
      {
        //Apply material
        glCallList( mat->matDL );
      }

      //Draw verts
      glDrawArrays( model->drawType, mat->start, mat->count);


      //Go to next material
      cMat++;
    } while( cMat< model->matCount );


  if(allWhite)
  {
    glEnable( GL_LIGHTING );
  }

}

void drawWireframeModel( vboModel* model, GLubyte c[4], int_fast8_t allWhite )
{


  glLineWidth(1.1f);
  glDisable(GL_CULL_FACE);
  glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  drawClayModel(model, c, allWhite);
  glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
  glEnable(GL_CULL_FACE);
  glLineWidth(1);
}

//Takes care of //freeing resources allocated to model (memory for struct, texture, buffers)
void eoModelFree( vboModel* model )
{
  int i;
  //Delete ogl buffer object
  glDeleteBuffers(1, &model->bufferName);

  //Free material names
  for( i=0; i<model->matCount; i++)
    free( model->materials[i].name );

  //Free materials
  free( model->materials );

  //free model texture name
  free( model->textureName );

  //Free model name
  free( model->name );

  //Free model dir
  free( model->dir );

  //Free model texture
  glDeleteTextures(1, &model->texture);

  //Free model struct
  free(model);
}

