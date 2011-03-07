/*
 * Preferences.cpp
 *
 * This file is a part of Tolmach project - the port of KDictionary
 * for BeOS/Haiku.
 *
 * copyright: (C) 1999 by Ivan V. Murasko
 *            (C) 2003-2004 by Zyozik (BeOS-related parts)
 *            (C) 2011 by Zyozik (Renewed for Haiku)
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

#include <Catalog.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Roster.h>
#include <unistd.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "TolmachPreferences"

const char* const cszPreferencesName = "Tolmach_Preferences";
//const char* const cszCurrentDictName = "CurrentDict";
const char* const cszWinState = "WinState";
const type_code ctcWinState = 'WNST';

TolmachApplication::Preferences::Preferences():BMessage()
{
  app_info appInfo;
  if(B_OK == be_app->GetAppInfo(&appInfo)){
    BEntry entry(&appInfo.ref);
    BEntry entryParent;
    entry.GetParent(&entryParent);
    entryParent.GetPath(&m_pathCurrent);
  }
  if(B_OK != m_pathCurrent.InitCheck()){
    char buf[B_PATH_NAME_LENGTH];
    m_pathCurrent.SetTo(getcwd(buf, B_PATH_NAME_LENGTH));
  }
  if(B_OK == find_directory (B_USER_SETTINGS_DIRECTORY, &m_pathPreferences)){
    m_pathPreferences.Append(cszPreferencesName);
  }else{
    m_pathPreferences = m_pathCurrent;
    m_pathPreferences.Append(cszPreferencesName);
  }
  Unflatten();
}

TolmachApplication::Preferences::~Preferences()
{
  Flatten();
}
        
status_t TolmachApplication::Preferences::Flatten()
{
  status_t status = B_ERROR;
  BFile file(m_pathPreferences.Path(), B_WRITE_ONLY|B_CREATE_FILE);
  if(B_OK == (status = file.InitCheck())){
    MakeEmpty();
    for(StateIterator i = m_states.begin(); i != m_states.end(); i++){
      WinState& state = *i;
	  status = AddFlat(cszWinState, &state);
    }
    status |= BMessage::Flatten(&file);
  }
  return status;
}

status_t TolmachApplication::Preferences::Unflatten()
{
  status_t status = B_ERROR;
  BFile file(m_pathPreferences.Path(), B_READ_ONLY);
  if(B_OK == (status = file.InitCheck())){
    if(B_OK == (status = BMessage::Unflatten(&file))){
      type_code tc = 0;
      int32 count = 0;
      if(B_OK == (status = GetInfo(cszWinState, &tc, &count))){
        for(int i=0; i < count; i++){
          WinState state;
          if(B_OK == (status = FindFlat(cszWinState, i, &state))){
            m_states.push_back(state);
          }
        }
      }
    }
  }
  return status;
}

TolmachApplication::Preferences::WinState::WinState()
		: m_rect(0,0,0,0), m_path(""), m_bReverse(false)
{
}

void
TolmachApplication::Preferences::
  WinState::Set(const BRect &rect, const BPath &path, bool bReverse)
{
  m_rect = rect;
  m_path = path;
  m_bReverse = bReverse;
}
          
bool
TolmachApplication::Preferences::WinState::IsFixedSize() const
{
  return false;
}

type_code
TolmachApplication::Preferences::WinState::TypeCode() const
{
  return ctcWinState;
}

ssize_t
TolmachApplication::Preferences::WinState::FlattenedSize() const
{
  return sizeof(BRect) + sizeof(bool) + m_path.FlattenedSize();
}

status_t
TolmachApplication::Preferences::WinState::Flatten(void *buffer, ssize_t size) const
{
  status_t status = B_ERROR;
  ssize_t req_size = FlattenedSize();
  if(size >= req_size){
    ssize_t sz = sizeof(BRect);
    memcpy(buffer, &m_rect, sz);
    char *ptr = (char *)buffer;
    ptr += sz;
    size -= sz;
    sz = sizeof(bool);
    memcpy(ptr, &m_bReverse, sz);
    ptr += sz;
    size -= sz;
    status = m_path.Flatten(ptr, size);
  }
  return status;
}

bool
TolmachApplication::Preferences::WinState::AllowsTypeCode(type_code code) const
{
  return code == ctcWinState;
}

status_t
TolmachApplication::Preferences::WinState::Unflatten(type_code c,
												const void *buf, ssize_t size)
{
  status_t status = B_ERROR;
  if(AllowsTypeCode(c)){
    ssize_t sz = sizeof(BRect);
    memcpy(&m_rect, buf, sz);
    const char *ptr = (const char *)buf;
    ptr += sz;
    size -= sz;
    sz = sizeof(bool);
    memcpy(&m_bReverse, ptr, sz);
    ptr += sz;
    size -= sz;
    status = m_path.Unflatten(m_path.TypeCode(), ptr, size);
  }
  return status;
}

