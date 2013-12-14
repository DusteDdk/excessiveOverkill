#include "sceneLoader.h"

listItem* eoLoadScene( const char* fileName, engObjInitFunc objInit )
{
  char line[2048];
  char set[1024];
  char val[1024];
  listItem* objs = NULL;
  engObj_s* obj = NULL;
  int i=0;
  char **vec;

  FILE* fp = fopen( fileName, "r" );
  //Open file
  if( fp )
  {
    objs = initList();
    eoPrint("Loading %s ...", fileName);
    while( fgets(line, 1023, fp) )
    {
      stripNewLine(line);

      if( strlen(line) > 2 )
      {
        if( strcmp(line,"[model]") == 0 )
        {
          obj = eoObjCreate( ENGOBJ_MODEL );
        } else if( strcmp(line,"[sprite]" ) == 0 )
        {
          eoPrint("Created Sprite entity.");
          obj = eoObjCreate( ENGOBJ_SPRITE );
        } else if( strcmp(line,"[emitter]") == 0 )
        {
          eoPrint("Created Emitter entity.");
          obj = eoObjCreate( ENGOBJ_PAREMIT );
        } else if( strcmp(line, "[sound]" ) == 0 )
        {
          eoPrint("Created Sound entity.");
          obj = eoObjCreate( ENGOBJ_SOUND );
        } else if( strcmp(line,"[end]") == 0 )
        {
          if( objInit )
          {
            objInit(obj);
          }
          listAddData( objs, (void*)obj );
        } else {
          if( splitVals( '=', line, set, val ) )
          {
            if( strcmp(set,"class") == 0 )
            {
              obj->className = malloc(strlen(val)+1);
              strcpy(obj->className,val);
            } else if( strcmp(set,"rot") == 0 )
            {
              vec = explode(',', val, 3);
              obj->rot.x=atof(vec[0]);
              obj->rot.y=atof(vec[1]);
              obj->rot.z=atof(vec[2]);

              free(vec[0]);
              free(vec[1]);
              free(vec[2]);
              free(vec);
            } else if( strcmp(set,"pos") == 0 )
            {
              vec = explode(',', val, 3);
              obj->pos.x=atof(vec[0]);
              obj->pos.y=atof(vec[1]);
              obj->pos.z=atof(vec[2]);

              free(vec[0]);
              free(vec[1]);
              free(vec[2]);
              free(vec);
            } else if( strcmp(set,"file") == 0 )
            {
              //Todo: Cache models
              i=charrpos(val,'/')+1;

              strcpy(set, val+i);
              val[i]=0;
              obj->model = eoModelLoad( val, set );

            } else {
              eoPrint("Unknown data: %s ('%s' and '%s')",line,set,val);
            }

          } else
            eoPrint("Invalid text: %s",line);
        }
    }
    //  free(objName);
    }

  } else {
    eoPrint("Error, could not open file %s for reading...", fileName);
  }

  return(objs);
}


