/*
 * PGBIndex.cpp
 *
 * This file is a part of Tolmach project - the port of KDictionary
 * for BeOS.
 *
 * copyright: (C) 1999 by Ivan V. Murasko
 *            (C) 2003-2004 by Zyozik (BeOS-related parts)
 */
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "TolmachApp.h"
#include "PGBHandler.h"
#include <Catalog.h>
#include <Locale.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "PGBIndex"

PGBHandler::PGBIndex::PGBIndex(BFile &fileDict):m_fileDict(fileDict),m_aIndex(0)
{
  //m_aIndex.resize(1);
  m_aIndex = new unsigned char[1];
  m_nElements = 1;
}

PGBHandler::PGBIndex::PGBIndex(BFile &fileDict, int position, int number)
                        :m_fileDict(fileDict), m_nElements(number),m_aIndex(0)
{
  int size = m_nElements * 10;
  delete[] m_aIndex;
  m_aIndex = new unsigned char[size];
  //m_aIndex.resize(size);
  status_t status = B_OK;
  if(size != (status = m_fileDict.ReadAt(position, /*&m_aIndex[0]*/m_aIndex, size))){
    //m_aIndex.resize(1);
    delete[] m_aIndex;
    m_aIndex = new unsigned char[1]; 
    m_nElements = 1;
    theApp.ShowAlert(B_TRANSLATE("Error"), B_TRANSLATE("Error reading PGB index"),
		   B_STOP_ALERT, status);
  }
}

PGBHandler::PGBIndex::~PGBIndex()
{
}
        
int
PGBHandler::PGBIndex::GetWord(const int &pos)
{
  unsigned char* tmpPointer;
  tmpPointer=/*&m_aIndex[10*pos]*/m_aIndex + 10 * pos; //??? optimize
  void* pointerVoid;
  pointerVoid=tmpPointer;
  return (*(int*) pointerVoid);
}

int
PGBHandler::PGBIndex::GetTranslation(const int &pos)
{
  unsigned char* tmpPointer;
  tmpPointer=/*&m_aIndex[10*pos+4]*/ m_aIndex + 10 * pos + 4; //??? optimize
  void* pointerVoid;
  pointerVoid=tmpPointer;
  return (*(int*) pointerVoid);
}

unsigned char
PGBHandler::PGBIndex::GetFirstLetter(const int &pos)
{
  unsigned char* value=/*&m_aIndex[10*pos+8]*/m_aIndex + 10 * pos + 8;
  return *value;
}

unsigned char
PGBHandler::PGBIndex::GetLastLetter(const int &pos)
{
  unsigned char* value=/*&m_aIndex[10*pos+9]*/ m_aIndex + 10 * pos + 9;
  return *value;
}

int
PGBHandler::PGBIndex::GetNumberWords()
{
  unsigned char* charPointer;
  void* pointerVoid;
  int numberWords;
  numberWords=0;
  charPointer=/*&m_aIndex[4]*/m_aIndex + 4;
  for (int i = 0; i < m_nElements; i++){
    pointerVoid=charPointer;
    if ((*(int*) pointerVoid)>0)
      numberWords++;
    charPointer+=10;
  }
  return numberWords;
}

void
PGBHandler::PGBIndex::FindNext(int &j)
{
  unsigned char* charPointer;
  void* pointerVoid;
  j++;
  charPointer=/*&m_aIndex[10*j+4]*/m_aIndex + 10 * j + 4;
  pointerVoid=charPointer;
  while ((*(int*) pointerVoid)<0){
    if (j == m_nElements)
      break;
    charPointer += 10;
    pointerVoid = charPointer;
    j++;
  }
}

