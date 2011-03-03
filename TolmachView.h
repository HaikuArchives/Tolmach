/*
 * TolmachView.h
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

#ifndef _TOLMACHVIEW_H_
#define _TOLMACHVIEW_H_

#include <list>
#include <View.h>
#include <TextControl.h>
#include <ListView.h>
#include <TextView.h>

class TolmachView : public BView
{
    struct StyleItem{
      bool  bBold;
      int32 start;
      int32 end;
    };
    std::list<StyleItem> aStyleItems;
  public:
    BTextControl *m_pWordEdit;
    BListView *m_pWordsList;
    BScrollView *m_pWordsListScrollView;
    BTextView *m_pTransView;

  public:
    TolmachView(uint32 resizing_mode);
    virtual void AllAttached(void);
    virtual void FrameResized(float width, float height);
    
    void ResetStyleArray(); 
    void AppendStyleItem(bool bBold, int32 start, int32 length);
    void ApplyStyleArray();
};

#endif //_TOLMACHVIEW_H_

