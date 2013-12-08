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

#ifndef ENG_H_INCLUDED
#define ENG_H_INCLUDED

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "gl.h"
#include "eng.h"
#include "gfxeng.h"
#include "console.h"
#include "vboload.h"
#include "tick.h"
#include "sound.h"
#include "camera.h"
#include "sprite.h"
#include "strings.h"
#include "version.h"
#include "particles.h"
#include "game.h"
#include "input.h"
#include "gui.h"

#include "types.h"

#include "list.h"
#include "data.h"
#include "sceneLoader.h"
int eoInitAll(int argc, char** argv, const char* datadir);
void eoMainLoop();
void eoQuit();

#endif // ENG_H_INCLUDED
