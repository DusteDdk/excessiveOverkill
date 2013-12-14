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

#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

void stripNewLine(char* str);
void stripTabs(char* str);
int splitVals(char ch,const char* buf, char* set, char* val); //Splits a setting=value line and returns true if it did. else returns 0
int charrpos(const char* str, char c); //Return position of last instance of character c

 //Splits a string into several strings between any number of delim characters.
 //Stops when size is met or when no more delimiters are found.
 //If a string is not used the ptr is 0
char** explode(char delim, char* buf, int size);
#endif // STRINGS_H_INCLUDED
