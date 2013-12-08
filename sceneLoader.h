#ifndef SCENELOADER_INCLUDED
#define SCENELOADER_INCLUDED
#include "eng.h"

typedef void (*engObjInitFunc) (engObj_s*);

//Returns a list of unbaked engObj_s*, calling the init function for each
//object if init function is not null.
//Returns NULL if file could not be opened.
listItem* eoLoadScene( const char* fileName, engObjInitFunc objInit );

#endif
