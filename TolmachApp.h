/*
 * TolmachApp.h
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

#ifndef _TOLMACHAPP_H_
#define _TOLMACHAPP_H_

#include <Application.h>
#include <Alert.h>
#include <Path.h>
#include <Flattenable.h>
#include <String.h>
#include <vector>
#include <algorithm>

class TolmachWindow;

enum { 
	eOLng = 0,
	eDLng = 1,
	eCountLng 
};
  
struct DictDescription
{
  BString name;
  BPath   path;
  BString langs[eCountLng];
  TolmachWindow *wins[eCountLng];
  DictDescription(){
    langs[eOLng] = "Source Lang.";
    langs[eDLng] = "Dest. Lang.";
    wins[eOLng] = wins[eDLng] = 0;
  }  
};

class TolmachApplication : public BApplication
{
    class Preferences : public BMessage {
        BPath m_pathPreferences;
        status_t Flatten();
        status_t Unflatten();
      public:
        BPath m_pathCurrent;
        //BPath m_pathCurrentDict;
        class WinState : public BFlattenable{
        public:
          bool m_bReverse;
          BPath m_path;
          BRect m_rect;
          WinState();
          void Set(const BRect &rect, const BPath &path, bool m_bReverse);
          virtual bool      IsFixedSize() const;
          virtual type_code TypeCode() const;
          virtual ssize_t   FlattenedSize() const;
          virtual status_t  Flatten(void *buffer, ssize_t size) const;
          virtual bool      AllowsTypeCode(type_code code) const;
          virtual status_t  Unflatten(type_code c, const void *buf, ssize_t size);
        }m_wsLastClosed;
      typedef std::vector<WinState> StateVector;  
      typedef std::vector<WinState>::iterator StateIterator;
      StateVector m_states;  
      public:
        Preferences();
        ~Preferences();
    }m_Preferences;

  typedef std::vector<DictDescription> DictVector;
  typedef std::vector<DictDescription>::iterator DictIterator;

    DictVector m_dicts;
    BRect m_rcBounds;
    
    //status_t m_statusInit;
  typedef std::vector<int> Dict2LoadVector;
  Dict2LoadVector m_dicts2load;  
 
    void OffsetNextBounds(TolmachWindow *pLatestWin);
    static void UpdateEachMenu(DictDescription &dd);
    void ProceedCmdArguments(std::vector<entry_ref>& entries);

    void LoadDictList();
	int  LoadDictFile(BEntry& entry);
    void UpdateMIMETypes();
    void LoadWinStates();
  public:
    TolmachApplication();

    virtual void ReadyToRun(void);
    virtual void AboutRequested(void);
    virtual bool QuitRequested(void);
    virtual void RefsReceived(BMessage *msg);
    virtual void ArgvReceived(int32 argc, char **argv);
    void WindowCloseRequested(int nDict, bool bReverse);
//    virtual void MessageReceived(BMessage *message);

    status_t GetDictAt(DictDescription *ddescr, int idx) const;
    int DictCount() const;
    void ShowDictWindow(int nDict, bool bReverse, bool bUpdateMenus = true,
                                                     BRect *rect = 0);
    
    void ShowAlert(const char* title, const char *message,
                   alert_type type = B_INFO_ALERT, status_t status = B_OK);
    void ShowAlert(const BString &strTitle, const BString &strMessage,
                   alert_type type = B_INFO_ALERT, status_t status = B_OK);
};

extern TolmachApplication  theApp;

#endif //_TOLMACHAPP_H_

