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

#include "data.h"
#include <stdio.h>
#include <string.h>
#include "console.h"

static char buf[2048];

static char datroot[1024];


const char* Data( const char* dir, const char* file )
{
  sprintf( buf, "%s%s%s", datroot, dir,file );
  return(buf);
}


void DataSetDir( const char* dir )
{
  if( strlen(dir) > 1023 )
  {
    strcpy( datroot, "./");
    eoPrint("Can't use datadir: '%s' with length of %i characters. Max characters in datapath is 1023", dir, strlen(dir) );
  }
  else
  {
    eoPrint("Datadir: '%s'",dir);
    strcpy( datroot, dir );
  }
}

