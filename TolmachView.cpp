/*
 * TolmachView.cpp
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
#include <StringView.h>
#include <Box.h>
#include <ScrollView.h>
#include <SplitView.h>
#include <Catalog.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <stdio.h>

#include "TolmachView.h"
#include "Constants.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "TolmachView"

const float cfVertSpace = 10.f;
const float cfHorzSpace = 10.f;
const float cfWordsWidth  = 400.f;
const float cfWordsHeight  = 150.f;
const float cfTransHeight = 170.f;

TolmachView::TolmachView(uint32 resizingMode)
             :BView(BRect(0, 0, 0, 0), "Tolmach View",
                    resizingMode, B_WILL_DRAW | B_FRAME_EVENTS),
              m_pWordEdit(0), m_pWordsList(0), m_pTransView(0),
              m_pWordsListScrollView(0)
{
  SetViewColor(BKG_GREY);
}

void TolmachView::AllAttached(void)
{
    //word edit control
  BRect rect(cfHorzSpace, cfVertSpace, cfWordsWidth, 0);
  m_pWordEdit = new BTextControl(rect, "WordEdit", B_TRANSLATE("Word(s):"), "",
                                 0, B_FOLLOW_NONE);
  AddChild(m_pWordEdit);
  m_pWordEdit->ResizeToPreferred();
  m_pWordEdit->SetModificationMessage(new BMessage(MSG_EDIT_CHANGE));
  float fWordHeight = m_pWordEdit->Bounds().Height();
  m_pWordEdit->ResizeTo(cfWordsWidth, fWordHeight);

    //words list view
  rect.Set(0, 0, cfWordsWidth - B_V_SCROLL_BAR_WIDTH, cfWordsHeight);  
  m_pWordsList = new BListView(rect, "WordsList");
  m_pWordsList->MoveTo(cfHorzSpace, fWordHeight + cfVertSpace * 2);
  m_pWordsListScrollView =
                  new BScrollView("ScrollWords", m_pWordsList,
                                     B_FOLLOW_NONE, 0, false, true);
  AddChild(m_pWordsListScrollView);

  m_pWordsList->SetSelectionMessage(new BMessage(MSG_LIST_CHANGE));
  m_pWordsList->SetInvocationMessage(new BMessage(MSG_LIST_INVOKE));
    //translation text view
  rect.Set(0, 0, cfWordsWidth - B_V_SCROLL_BAR_WIDTH, cfTransHeight);
  m_pTransView = new BTextView(rect, "TransView", rect,
                                 B_FOLLOW_NONE, B_NAVIGABLE | B_WILL_DRAW);
  m_pTransView->MoveTo(cfHorzSpace ,
                       fWordHeight + cfWordsHeight + cfVertSpace * 3);
  BScrollView *pTransScrollView =
                  new BScrollView("TransView", m_pTransView,
                     B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, true, true);
  AddChild(pTransScrollView);
  m_pTransView->MakeEditable(false);
  ResizeTo(cfWordsWidth + cfHorzSpace * 2,
             cfTransHeight + fWordHeight + cfWordsHeight +
                    B_H_SCROLL_BAR_HEIGHT + cfVertSpace * 4);
/*
  BSplitView* splitView = new BSplitView(B_HORIZONTAL);
  BLayoutBuilder::Split<>(splitView)
				.Add(m_pWordsListScrollView)
				.Add(pTransScrollView);
  AddChild(splitView);
 */ 
  m_pWordEdit->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
  m_pWordsListScrollView->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP_BOTTOM);
  m_pWordsList->SetResizingMode(B_FOLLOW_ALL);
  pTransScrollView->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM);
  m_pTransView->SetResizingMode(B_FOLLOW_ALL);
  m_pTransView->SetStylable(true);
  
  m_pWordEdit->MakeFocus(true);
}

void TolmachView::FrameResized(float width, float height)
{
  BView::FrameResized(width, height);
  m_pTransView->SetTextRect(BRect(0, 0, m_pTransView->Bounds().Width(), 0));
}

void TolmachView::ResetStyleArray()
{
  aStyleItems.clear();
  BFont font;
  GetFont(&font);
  font.SetFace(B_REGULAR_FACE);
  m_pTransView->SetFontAndColor(0, -1, &font);
}

void TolmachView::AppendStyleItem(bool bBold, int32 start, int32 off)
{
   StyleItem si;
   si.bBold = bBold;
   si.start = start;
   si.end = start + off;
   aStyleItems.push_back(si);
}

void TolmachView::ApplyStyleArray()
{
  BFont fontBold;
  GetFont(&fontBold);
  fontBold.SetFace(B_BOLD_FACE | B_ITALIC_FACE);
  BFont fontHiLight;
  GetFont(&fontHiLight);
  fontHiLight.SetFace(B_BOLD_FACE);
  rgb_color rgbBold = make_color(0, 0, 128);
  rgb_color rgbHiLight = make_color(0, 128, 0);

  //aStyleItems.sort();
  for(std::list<StyleItem>::iterator i = aStyleItems.begin();
                                      i != aStyleItems.end(); i++){
/*	printf("apply0:%d %d-%d\n", i->line, i->start, i->end);
	int32 lineStart = m_pTransView->OffsetAt(i->line);
	int32 start = lineStart + i->start;
	int32 end = lineStart + i->end;
	if(i->end == -1) {
		int32 nextLine = i->line + 1;
		if(nextLine == m_pTransView->CountLines()) {
			end = m_pTransView->TextLength();
		} else {
			end = m_pTransView->OffsetAt(nextLine) - 1;
		}
	}
	
	printf("apply1:%d %d-%d\n", lineStart, start, end); */
	if (i->bBold) 
		m_pTransView->SetFontAndColor(i->start, i->end, &fontBold, B_FONT_ALL, &rgbBold);
	else
		m_pTransView->SetFontAndColor(i->start, i->end, &fontHiLight, B_FONT_ALL, &rgbHiLight);
  }
}

