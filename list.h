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

#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

/* These macros allow for syntax like
  {itBegin( type_t, elementName, listName)
    .. work with elementName here --
  itEnd();}
*/

#define itBegin(type, item, list) { listItem* it=list;while( (it=it->next) ) { type item = (type)it->data;
#define itEnd(); } }


typedef struct list
{
  void* data;
  struct list* next;
} listItem;

void listAddData(listItem* start, void* data);
void listInsertData(listItem* start, void* data, int p); //Inserts into the list at pos p. 0 = first
listItem* listRemovePos(listItem* start, int p); //Removes item from list, returns item just before removed item if any. 0=first
void listRemoveByData( listItem* start, void* data ); //Remove first item with said data
listItem* listRemoveItem(listItem* start, listItem* item); //Removes item from list, returns item just before removed item.
listItem* initList();
void freeList(listItem* start); //Only frees list, you still need to recurse through it and free data ptrs however they need to be.
int listSize(listItem* start);

int listToBuf( listItem* start, char* buf ); //Add each element in list to buffer, returns amount of items added.
listItem* listFromBuf( const char* buf );

void* listGetItemData(listItem* start, int index);


#endif // LIST_H_INCLUDED
