/*
 * TolmachWin.cpp
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

#include "TolmachWin.h"

#include <Application.h>
#include <Box.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <scheduler.h>

#include "TolmachApp.h"
#include "Constants.h"

const float cfVertSpace = 5.f;
const float cfHorzSpace = 5.f;

TolmachWindow::TolmachWindow(BRect frame, int nDict, bool bReverse)
               :BWindow(frame, "Tolmach", B_TITLED_WINDOW, 0),
               m_pMenuBar(0), m_pProgress(0),
               m_pStatusView(0), m_pDictMenu(0), m_PGBHandler(this),
			   m_selectWordInListWatchDog(0),
               m_nDict(nDict), m_bReverse(bReverse),m_tid(-1)
{
  initLayout();
}

TolmachWindow::~TolmachWindow()
{
} 

BTextControl*
TolmachWindow::WordEdit()
{
	return m_pWordEdit;
}

BListView*
TolmachWindow::WordsList()
{
	return m_pWordsList;
}


TolmachWindow::ArticleView*
TolmachWindow::TransView()
{
	return m_pTransView;
}


bool TolmachWindow::QuitRequested()
{
  bool bRet = true;
  thread_info ti;
  if(B_OK == get_thread_info(m_tid, &ti)){
    theApp.ShowAlert(B_TRANSLATE("Warning"),
			B_TRANSLATE("Unfortunately, it is not possible to close "
            "dictionary windows until finishing of initialization."), B_STOP_ALERT);
    bRet = false;
  } else
    theApp.WindowCloseRequested(m_nDict, m_bReverse);

  return bRet;
}

void TolmachWindow::initMenuBar()
{
  BRect rect(0, 0, 0, 0);
  m_pMenuBar = new BMenuBar(rect, "Tolmach Menu Bar");
   //dictionaries popup menu  
  BMenu *pTolmachMenu = new BMenu(B_TRANSLATE("Tolmach"));
  
  //Preferences menu item
/*  BMenuItem *pPrefsMenuItem = new BMenuItem("Preferences...",
                                  new BMessage(MSG_CMD_DICT_PREFS));
  pTolmachMenu->AddItem(pPrefsMenuItem);
  //About menu item
  pTolmachMenu->AddSeparatorItem(); */
  BMenuItem *pAboutMenuItem = new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS),
                                  new BMessage(B_ABOUT_REQUESTED));
  pAboutMenuItem->SetTarget(NULL, be_app);
  pTolmachMenu->AddItem(pAboutMenuItem);
  //Quit menu item
  pTolmachMenu->AddSeparatorItem();
  BMenuItem *pCloseMenuItem = new BMenuItem(B_TRANSLATE("Close"),
                                 new BMessage(B_QUIT_REQUESTED), 'W');
  pTolmachMenu->AddItem(pCloseMenuItem);
  BMenuItem *pQuitMenuItem = new BMenuItem(B_TRANSLATE("Quit"),
                                 new BMessage(MSG_CMD_QUIT_REQUESTED), 'Q');
  pTolmachMenu->AddItem(pQuitMenuItem);
  m_pMenuBar->AddItem(pTolmachMenu);

  UpdateDictMenu();   
}

void TolmachWindow::initLayout()
{
  initMenuBar();

  m_pWordEdit = new BTextControl(B_TRANSLATE("Word(s):\t"), "", 0);
  m_pWordEdit->SetModificationMessage(new BMessage(MSG_EDIT_CHANGE));
  m_pWordsList = new BListView(BRect(0, 0,  1, 1), "WordsList", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);

  m_pWordsList->SetSelectionMessage(new BMessage(MSG_LIST_CHANGE));
  m_pWordsList->SetInvocationMessage(new BMessage(MSG_LIST_INVOKE));

  m_pTransView = new ArticleView(BRect(0, 0, 1, 1), "TransView",
		  BRect(0, 0, Bounds().Width() - B_V_SCROLL_BAR_WIDTH - cfHorzSpace * 2.5f, 1), 
                                 B_FOLLOW_ALL, B_NAVIGABLE | B_WILL_DRAW);
  
  m_pStatusView = new BStringView(BRect(0,0,1,1), "Status View",
                                   B_TRANSLATE("Stattuz"), B_FOLLOW_ALL);
//  m_pStatusView->SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));

  BLayoutBuilder::Group<>(this, B_VERTICAL)
	  .Add(m_pMenuBar)
	  .AddGrid()
		.AddTextControl(m_pWordEdit, 0, 0)
	    .SetInsets(cfHorzSpace, 0., cfHorzSpace, 0.)
	    .End()
	  .AddGroup(B_VERTICAL)
	    .AddSplit(B_VERTICAL, 1.0)
		.Add(new BScrollView("ScrollWords", m_pWordsList, B_FOLLOW_NONE, 0, false, true))
		.Add(new BScrollView("TransView", m_pTransView, B_FOLLOW_NONE, 0, false, true))
	    .SetInsets(cfHorzSpace, 0., cfHorzSpace, 0.)
		.End()
	  .Add(m_pStatusView)
	    .End();

  m_pWordEdit->MakeFocus(true);

  float fMinW = 0.f, fMaxW = 0.f, fMinH = 0.f, fMaxH = 0.f;
  GetSizeLimits(&fMinW, &fMaxW, &fMinH, &fMaxH);
  fMinW = Bounds().Width();
  fMinH = Bounds().Height();
  SetSizeLimits(fMinW, fMaxW, fMinH, fMaxH);
  SetZoomLimits(fMinW, fMaxH);
}

void
TolmachWindow::LoadDictionary()
{
  //SetTitle(m_PGBHandler.DictNameString().String());
  //m_PGBHandler.SetCurrent(m_nDict, m_bReverse);
  int32 priority = suggest_thread_priority();
  m_tid = spawn_thread(LoadDictionary, "dict_loader", priority, (void *)this);
  resume_thread(m_tid);
}

int32
TolmachWindow::LoadDictionary(void *param)
{
  int ret = 0;
  TolmachWindow *pThis = static_cast<TolmachWindow *>(param);
  if(pThis){
    pThis->m_PGBHandler.SetCurrent(pThis->m_nDict, pThis->m_bReverse);
    //pWindow->PostMessage();   
  }else{
    //todo
    ret = -1;
  }
  return ret;
}


void TolmachWindow::UpdateDictMenu()
{
  int count = theApp.DictCount();
  BMenu *pNewMenu = new BMenu(B_TRANSLATE(B_TRANSLATE("Dictionaries")));
  if(count > 0){
    pNewMenu->AddSeparatorItem();
    int sep_index = 0;
    char shortcut = 'A';
    int msg_num = MSG_CMD_DICT_NUM;
    DictDescription dict;
    for(int i = 0; i < count; i++){
      if(B_OK == theApp.GetDictAt(&dict, i)){
        BString strOD(dict.langs[eOLng]); 
        strOD << " -> " << dict.langs[eDLng];
        BString strDO(dict.langs[eDLng]); 
        strDO << " -> " << dict.langs[eOLng];
        BString strODL(dict.name);
        strODL << " / " << strOD;
        BString strDOL(dict.name);
        strDOL << " / " << strDO;
        
        BMenu *pSubMenu = 0;
        BMenuItem *pMenuItem = new BMenuItem("", new BMessage(msg_num));
        if(dict.wins[eOLng]){
          bool me = (m_nDict == i) && m_bReverse == false;
          BString strLabel( !me ? B_UTF8_ELLIPSIS : "");
          strLabel << strODL;
          pMenuItem->SetLabel(strLabel.String());
          pMenuItem->SetShortcut(shortcut++, B_OPTION_KEY);
          pMenuItem->SetMarked(me);
          pNewMenu->AddItem(pMenuItem, sep_index++);
        }else{
          pSubMenu = new BMenu(dict.name.String());
          pNewMenu->AddItem(new BMenuItem(pSubMenu, new BMessage(msg_num)));
          pMenuItem->SetLabel(strOD.String());
          pSubMenu->AddItem(pMenuItem);
        }
        msg_num++;
        
        pMenuItem = new BMenuItem("", new BMessage(msg_num));
        if(dict.wins[eDLng]){
          bool me = (m_nDict == i) && m_bReverse == true;
          BString strLabel( !me ? B_UTF8_ELLIPSIS : "");
          strLabel << strDOL;
          pMenuItem->SetLabel(strLabel.String());
          pMenuItem->SetShortcut(shortcut++, B_OPTION_KEY);
          pMenuItem->SetMarked(me);
          pNewMenu->AddItem(pMenuItem, sep_index++);
        }else{
          if(!pSubMenu){
            pSubMenu = new BMenu(dict.name.String());
            pNewMenu->AddItem(new BMenuItem(pSubMenu, new BMessage(msg_num)));
          }
          pMenuItem->SetLabel(strDO.String());
          pSubMenu->AddItem(pMenuItem);
        }
        msg_num++;
      }
    }
  }else{
    BMenuItem *pNoDictMenuItem = new BMenuItem(B_TRANSLATE("<no dictionaries>"),
                                   new BMessage(MSG_CMD_DICT_NONE));
    pNewMenu->AddItem(pNoDictMenuItem);
  }  
  m_pMenuBar->RemoveItem(m_pDictMenu);
  m_pMenuBar->AddItem(pNewMenu);
  delete m_pDictMenu;
  m_pDictMenu = pNewMenu;
}

void TolmachWindow::MessageReceived(BMessage *message)
{
	int64 when = 0;
  int idx = message->what - MSG_CMD_DICT_NUM;
  if(idx >= 0 && idx < theApp.DictCount() * 2){
    theApp.ShowDictWindow(idx / 2, 0 != idx % 2);
  }else{
    switch(message->what){
      case MSG_LIST_CHANGE:
        m_PGBHandler.WordHighlighted();
        break;
      case MSG_LIST_INVOKE:
        m_PGBHandler.WordListInvoked();
        break;
      case MSG_EDIT_CHANGE:
        m_PGBHandler.WordEditChanged(message);
        break;
      case MSG_CMD_QUIT_REQUESTED:
        be_app->PostMessage(B_QUIT_REQUESTED);
        break;
      case MSG_CMD_UPDATE_DICT_MENU:
        UpdateDictMenu();
        break;
      case MSG_CMD_DICT_NONE:
        theApp.ShowAlert(B_TRANSLATE("Warning"),
			   B_TRANSLATE("No dictionaries available. =-("));
        break;
      case MSG_CMD_LOAD_CURRENT_DICT:
        LoadDictionary();
        break;
      default:
        BWindow::MessageReceived(message);
        break;
    }
  }
}

void TolmachWindow::DispatchMessage(BMessage *msg, BHandler *target)
{
  BWindow::DispatchMessage(msg, target);
  if(msg->what == B_KEY_DOWN){
    if(m_pWordEdit->ChildAt(0) == target)
      HandleWordEditKeyDown(msg);
  }
}

void
TolmachWindow::FrameResized(float width, float height)
{
  BWindow::FrameResized(width, height);

//  BRect rc = m_pTransView->TextRect();
//  fprintf(stderr, "W:%f, %f, %f, %f\n", rc.left, rc.top, rc.right, rc.bottom);

//  rc. right = rc.left + m_pTransView->Bounds().Width();
//  m_pTransView->SetTextRect(rc);
}

void TolmachWindow::HandleWordEditKeyDown(BMessage *message)
{
	int32 nKey = 0;
	status_t st = message->FindInt32("raw_char", &nKey);
	if (B_OK != st) {
		fprintf(stderr, "raw_char field not found: %s\n", strerror(st));
		return;
	}
	
	int min = 0;
	int idx = m_pWordsList->CurrentSelection();
	int max = m_pWordsList->CountItems() - 1;
	switch (nKey) {
		case B_UP_ARROW:
			idx--;
			break;
		case B_DOWN_ARROW:
			idx++;
			break;
		case B_HOME:
			idx = min;
			break;
		case B_END:
			idx = max;
			break;
		case B_PAGE_UP:
		case B_PAGE_DOWN:
			{
				int pageCount = m_pWordsList->Bounds().Height()/m_pWordsList->ItemFrame(0).Height();
				idx += (nKey == B_PAGE_UP) ? -pageCount : pageCount;
			}
			break;
		case B_ENTER:
			{
				char ch = static_cast<char>(nKey);  
				m_pWordsList->BListView::KeyDown(&ch, 1);
			}
			break;

		default:
			return;
	}
	
	idx = max_c(min, idx);
	idx = min_c(max, idx);

	m_pWordsList->Select(idx);
	m_pWordsList->ScrollToSelection();

	idx = m_pWordsList->CurrentSelection();
	
	BStringItem *pItem = static_cast<BStringItem *>(m_pWordsList->ItemAt(idx));
	if (pItem != 0) {
		m_pWordEdit->SetText(BString(pItem->Text()).Trim());
		SetSelectWordInListWatchDog(system_time());
	}
}

void
TolmachWindow::SelectWordInList(int index)
{
  m_pWordsList->Select(index);
  m_pWordsList->ScrollToSelection();
}

void
TolmachWindow::SetSelectWordInListWatchDog(bigtime_t time)
{
  m_selectWordInListWatchDog = time;
}

bigtime_t
TolmachWindow::SelectWordInListWatchDog()
{
	return m_selectWordInListWatchDog;
}

TolmachWindow::ArticleView::ArticleView(BRect frame, const char* name, BRect textRect,
						uint32 resizingMode, uint32 flags)
				: BTextView(frame, name, textRect, resizingMode, flags)
{
//  SetWordWrap(false);
  MakeEditable(false);
  SetStylable(true);
}

void
TolmachWindow::ArticleView::FrameResized(float width, float height)
{
//  BRect rc = TextRect();
//  fprintf(stderr, "%f, %f, %f, %f\n", rc.left, rc.top, rc.right, rc.bottom);
  //rc.right = rc.left + width;
  //rc.right++;
//  SetTextRect(rc);
  BTextView::FrameResized(width, height);
}

void TolmachWindow::ArticleView::ResetStyleArray()
{
  aStyleItems.clear();
  BFont font;
  GetFont(&font);
  font.SetFace(B_REGULAR_FACE);
  SetFontAndColor(0, -1, &font);
}

void TolmachWindow::ArticleView::AppendStyleItem(bool bBold, int32 start, int32 off)
{
   StyleItem si;
   si.bBold = bBold;
   si.start = start;
   si.end = start + off;
   aStyleItems.push_back(si);
}

void TolmachWindow::ArticleView::ApplyStyleArray()
{
  BFont fontBold;
  GetFont(&fontBold);
  fontBold.SetFace(B_BOLD_FACE | B_ITALIC_FACE);
  BFont fontHiLight;
  GetFont(&fontHiLight);
  fontHiLight.SetFace(B_BOLD_FACE);
  rgb_color rgbBold = make_color(0, 0, 128);
  rgb_color rgbHiLight = make_color(0, 128, 0);

  for(std::list<StyleItem>::iterator i = aStyleItems.begin();
                                      i != aStyleItems.end(); i++){
	if (i->bBold) 
		SetFontAndColor(i->start, i->end, &fontBold, B_FONT_ALL, &rgbBold);
	else
		SetFontAndColor(i->start, i->end, &fontHiLight, B_FONT_ALL, &rgbHiLight);
  }
}

