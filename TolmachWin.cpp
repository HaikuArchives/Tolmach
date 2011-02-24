/*
 * TolmachWin.cpp
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
#include <Application.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <String.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Box.h>
#include <scheduler.h>
#include <Catalog.h>
#include <Locale.h>

#include "TolmachApp.h"
#include "TolmachWin.h"
#include "TolmachView.h"
#include "Constants.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "TolmachWindow"

TolmachWindow::TolmachWindow(BRect frame, int nDict, bool bReverse)
               :BWindow(frame, "Tolmach", B_TITLED_WINDOW, 0),
               m_pMenuBar(0), m_pTolmachView(0), m_pProgress(0),
               m_pStatusView(0), m_pDictMenu(0), m_PGBHandler(this),
               m_nDict(nDict), m_bReverse(bReverse),m_tid(-1)
{
  initLayout();
}

TolmachWindow::~TolmachWindow()
{
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
  }else
    theApp.WindowCloseRequested(m_nDict, m_bReverse);
  return(bRet);
}

void TolmachWindow::initLayout()
{
  initMenuBar();
  
  float menuHeight = m_pMenuBar->Bounds().Height();
    // Menu view
  m_pTolmachView = new TolmachView(B_FOLLOW_NONE); // don't follow window just yet!
  m_pTolmachView->MoveBy(0, menuHeight + 1);
  AddChild(m_pTolmachView);
  
    // Status view	
  BRect Rect = m_pTolmachView->Frame();
  float top = Rect.bottom + 1;
  font_height plainHeight;
  be_plain_font->GetHeight(&plainHeight);
	
  BRect boxFrame;
  boxFrame.Set(Rect.left - 2, top, Rect.right + 2,
               top + plainHeight.ascent + plainHeight.descent +
                                          plainHeight.leading + 4);

  BBox* pStatusBox = new BBox(boxFrame);
  AddChild(pStatusBox);

  BRect statusFrame = pStatusBox->Bounds();
  statusFrame.InsetBy(2,2);	
  m_pStatusView = new BStringView(statusFrame, "Status View",
                                   B_TRANSLATE("Stattuz"), B_FOLLOW_ALL);
  m_pStatusView->SetViewColor(BKG_GREY);
  pStatusBox->AddChild(m_pStatusView);
  
  float windowWidth = m_pTolmachView->Frame().right;
  float windowHeight = boxFrame.bottom - 4;
  ResizeTo(windowWidth, windowHeight);
  
  float fMinW,fMaxW,fMinH,fMaxH;
  GetSizeLimits(&fMinW,&fMaxW,&fMinH,&fMaxH);
  fMinW = windowWidth;
  fMinH = windowHeight;
  SetSizeLimits(fMinW, fMaxW, fMinH, fMaxH);
  SetZoomLimits(fMinW, fMaxH);
  
  m_pTolmachView->SetResizingMode(B_FOLLOW_TOP_BOTTOM|
                                   B_FOLLOW_LEFT_RIGHT);
  pStatusBox->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM);                                 
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
  BMenuItem *pAboutMenuItem = new BMenuItem(B_TRANSLATE("About..."),
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
  AddChild(m_pMenuBar);
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
        m_PGBHandler.WordEditChanged();
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
 /* if(msg->what == B_KEY_DOWN){
    if(m_pTolmachView->m_pWordEdit->ChildAt(0) == target)
      HandleWordEditKeyDown(msg);
  }*/
}

void TolmachWindow::HandleWordEditKeyDown(BMessage *msg)
{
  int32 nKey = 0;
  if(B_OK == msg->FindInt32("raw_char", &nKey)){
    BListView *pWordsList = m_pTolmachView->m_pWordsList;
    BTextControl *pWordEdit = m_pTolmachView->m_pWordEdit;
    bool bIsNavigationKey = true;
   // int idx = pWordsList->CurrentSelection();
    switch(nKey){
      case B_UP_ARROW:
   //     idx--;
   //     break;
      case B_DOWN_ARROW:
   //     idx++;
   //     break;
      case B_HOME:
   //     idx = 0;
   //     break;
      case B_END:
   //     idx = pWordsList->CountItems();
   //     break;
      case B_PAGE_UP:
      case B_PAGE_DOWN:/*{
          BScrollView *pWordsListScrollView = m_pTolmachView->m_pWordsListScrollView;
          BScrollBar *pScrollBar = pWordsListScrollView->ScrollBar(B_VERTICAL);
          if(pScrollBar){
            float fSmallStep = 1.0;
            float fBigStep = 10.0;
            pScrollBar->GetSteps(&fSmallStep, &fBigStep);
            float fScrollValue = pScrollBar->Value();
            bool bUp = (nKey == B_PAGE_UP);
            fScrollValue += bUp ? -fBigStep : fBigStep;
            idx = pWordsList->IndexOf(BPoint(0, fScrollValue));
            if(idx < 0){
              idx = bUp ? 0 : pWordsList->CountItems();
            }
          }
        }
        break;*/
      case B_ENTER:{
          char ch = static_cast<char>(nKey);  
          pWordsList->BListView::KeyDown(&ch, 1);
        }
        break;
      default:
         bIsNavigationKey = false;
         break;   
    }
    if(bIsNavigationKey){
      //pWordsList->Select(idx);
      //pWordsList->ScrollToSelection();
      int idx = pWordsList->CurrentSelection();
      BStringItem *pItem = static_cast<BStringItem *>(pWordsList->ItemAt(idx));
      if(pItem != 0){
        pWordEdit->SetText(pItem->Text());
        BTextView *pView = static_cast<BTextView *>(pWordEdit->ChildAt(0));
        pView->SelectAll();
      }
    }  
  }  
}

