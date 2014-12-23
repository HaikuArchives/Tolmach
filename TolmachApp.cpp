/*
 * TolmachApp.cpp
 *
 * This file is a part of Tolmach project - the port of KDictionary
 * for BeOS / Haiku.
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

#include <sys/stat.h>

#include <Alert.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <KernelExport.h>
#include <Mime.h>
#include <Resources.h>
#include <Screen.h>
#include <fs_attr.h>

#include "Constants.h"
#include "TolmachWin.h"

const BPoint ptNewWindowOffset(20.f, 20.f);
const BRect  rcInitBounds(100.f, 80.f, 400.f, 500.f);

TolmachApplication  theApp;

int main(int argc, char** argv)
{
  theApp.Run();
  return(0);
}

TolmachApplication::TolmachApplication()
                    :BApplication(cszApplicationSignature),
                    m_rcBounds(rcInitBounds)
{
  B_TRANSLATE_MARK_SYSTEM_NAME("Tolmach");

  UpdateMIMETypes();
  LoadDictList();
}

void
TolmachApplication::ReadyToRun(void)
{
  LoadWinStates();
}

void
TolmachApplication::UpdateEachMenu(DictDescription &dd)
{
  for(int i = 0; i < eCountLng; i++)
    if(dd.wins[i]){
      BMessage msg(MSG_CMD_UPDATE_DICT_MENU);
      dd.wins[i]->PostMessage(&msg);
    }
}

void
TolmachApplication::LoadWinStates()
{
  // in case some dicts were already opened as result of RefsReceived
  std::vector<TolmachWindow*> wins;
  for(DictIterator i = m_dicts.begin(); i != m_dicts.end(); i++){
    for(int j = 0; j < eCountLng; j++){
      if(0 != i->wins[j]){
		wins.push_back(i->wins[j]);
      }
    }
  }  

  int nWindowsCount = wins.size();
  if(0 != m_Preferences.m_states.size()){
    Preferences::StateIterator s = m_Preferences.m_states.begin();
    for(; s != m_Preferences.m_states.end(); s++){
      for(int d_idx = 0; d_idx < m_dicts.size(); d_idx++ ){
        DictDescription &dd = m_dicts[d_idx];
        if(dd.path == s->m_path){
          ShowDictWindow(d_idx, s->m_bReverse, false, &s->m_rect);
          nWindowsCount++;
          break;
        }
      }
    }
    for_each(m_dicts.begin(), m_dicts.end(), UpdateEachMenu);
  }

  if(0 == nWindowsCount) {
	if(m_dicts.size() > 0)
	  ShowDictWindow(0, false); // no windows opened or no states saved ... first run?
	else {
	  be_app->PostMessage(B_QUIT_REQUESTED);
	}
  } else
  if (wins.size() > 0) {
	std::vector<TolmachWindow*>::iterator i = wins.begin();
	for (; i != wins.end(); i++)
	  (*i)->Activate(true);
  }
}

void
TolmachApplication::ShowDictWindow(int nDict, bool bReverse, bool bUpdateMenus, BRect *rect)
{
  TolmachWindow *&win = m_dicts[nDict].wins[(bReverse ? eDLng: eOLng)];
  if(0 == win){
    win = new TolmachWindow(rect ? *rect : m_rcBounds, nDict, bReverse);
    if(!rect)
      OffsetNextBounds(win);
    win->Show();
#if 0
	win->Lock();
	BMessage msg2;
	win->Archive(&msg2, true);
	BFile file("win.msg", B_CREATE_FILE|B_WRITE_ONLY);
	msg2.Flatten(&file);
	//file.Close();
	win->Unlock();
#endif
    BMessage msg(MSG_CMD_LOAD_CURRENT_DICT);
    win->PostMessage(&msg);
    if(bUpdateMenus)
      for_each(m_dicts.begin(), m_dicts.end(), UpdateEachMenu);
  }else{
    win->Activate(true);
  }
}

void
TolmachApplication::OffsetNextBounds(TolmachWindow *pLatestWin)
{
	m_rcBounds.OffsetBy(ptNewWindowOffset);
	BPoint pt = m_rcBounds.LeftTop();
	pt.x += pLatestWin->Bounds().Width();
	pt.y += pLatestWin->Bounds().Height();
	m_rcBounds.SetRightBottom(pt);
	float fScreenWidth = BScreen(B_MAIN_SCREEN_ID).Frame().Width();
	float fScreenHeight = BScreen(B_MAIN_SCREEN_ID).Frame().Height();
	if (m_rcBounds.bottom > fScreenHeight)
		m_rcBounds.OffsetTo(m_rcBounds.left, 10);
	if (m_rcBounds.right > fScreenWidth)
		m_rcBounds.OffsetTo(6, m_rcBounds.top);
}

bool
TolmachApplication::QuitRequested(void)
{
	m_Preferences.m_states.clear();
	for(DictIterator i = m_dicts.begin(); i != m_dicts.end(); i++){
	  for(int j = 0; j < eCountLng; j++){
		if(0 != i->wins[j]){
		  Preferences::WinState state;
		  state.Set(i->wins[j]->Frame(), i->path, j != eOLng);
		  m_Preferences.m_states.push_back(state);
		}
	  }
	}  

	if(0 == m_Preferences.m_states.size()){
	  m_Preferences.m_states.push_back(m_Preferences.m_wsLastClosed);
	}

	return BApplication::QuitRequested();
}

void 
TolmachApplication::WindowCloseRequested(int nDict, bool bReverse)
{
  TolmachWindow *&win = m_dicts[nDict].wins[(bReverse ? eDLng : eOLng)];
  if(0 != win){
    m_Preferences.m_wsLastClosed.Set(win->Frame(), m_dicts[nDict].path, bReverse);
    win = 0;
  }
  for_each(m_dicts.begin(), m_dicts.end(), UpdateEachMenu);
  DictIterator i = m_dicts.begin();
  for(; i != m_dicts.end(); i++){
    if(0 != i->wins[eOLng] || 0 != i->wins[eDLng])
      break;
  }  
  if(i == m_dicts.end()){
    be_app->PostMessage(B_QUIT_REQUESTED);
  }
}

void
TolmachApplication::ProceedCmdArguments(std::vector<entry_ref>& entries)
{
  if (entries.size() <= 0)
    return;
  
  for (std::vector<entry_ref>::iterator i = entries.begin(); i != entries.end(); i++) {
    BPath path(&*i);
	BEntry entry(&*i);
	DictIterator d = m_dicts.begin();
	bool bOk = true;
    for (; d != m_dicts.end(); d++) {
      if(d->path == path) {
		ShowDictWindow(d - m_dicts.begin(), false);
        break;
	  }
      
	  int idx = LoadDictFile(entry);
	  if (idx >= 0) {
		ShowDictWindow(idx, false);
		break;
	  }
	  bOk = false;
	}

	if(!bOk) {
	  BString str(B_TRANSLATE("File '%file%' cannot be loaded."));
	  str.ReplaceFirst("%file%", path.Path());
	  BAlert* alert = new BAlert(B_TRANSLATE("Error"), str.String(), B_TRANSLATE("OK"));
	  alert->Go();
    }
  }
}

void
TolmachApplication::RefsReceived(BMessage *msg)
{
  entry_ref ref;
  std::vector<entry_ref> entries;
  int32 count = 0;
  type_code tc = 0;
  if(B_OK == msg->GetInfo("refs", &tc, &count)){
    entries.resize(count);
	for(int32 i = 0; i < count; i++) {
	  msg->FindRef((const char*)"refs", i, &entries[i]);
	}

    ProceedCmdArguments(entries);
  }
}

void
TolmachApplication::ArgvReceived(int32 argc, char **argv)
{
  std::vector<entry_ref> entries;
  for(int i = 1; i < argc; i++){
	entry_ref ref;
	status_t st = get_ref_for_path(argv[i], &ref);
	if(st == B_OK) {
	  entries.push_back(ref);
	} else
	  fprintf(stderr, B_TRANSLATE("File %s cannot be processed as dictionary file.\n%s\n"),
						argv[i], strerror(st));
  }

  if(entries.size() > 0)
	  ProceedCmdArguments(entries);
}

void TolmachApplication::AboutRequested(void)
{
  char szVVs[][8] = {"dev","alpha","beta","gamma","rc","release"};
  BString str("Tolmach");
  BResources *pResources = AppResources();
  size_t size = 0;
  int headerLen = 0;
  const void *pVersionData = pResources->LoadResource('APPV', 1, &size); 
  if(pVersionData){
    const struct version_info *pVI = (const struct version_info *)pVersionData;
    str = pVI->short_info;
	headerLen = str.Length();
	str << "\n\n" << B_TRANSLATE("Working shell for language bases "
								"in POLYGLOSSUM II (PGB) format.") << "\n";
    str << B_TRANSLATE("version");
   	str << " " << pVI->major << "." << pVI->middle << "." << pVI->minor
		<< "." << szVVs[pVI->variety] << "-" << pVI->internal << "\n\n";
                          
    str << B_TRANSLATE("Based on 'KDictionary' project.\n");
    str << B_TRANSLATE("Digged out for BeOS:\tNov.2003-Jan.2004.\n");
    str << B_TRANSLATE("Renewed for Haiku:\tFeb.2011-Mar.2011.\n\n");
    str << B_TRANSLATE("This application is distributed under the terms of "
			"General Public License. Look in sources for more details.\n");
  }

  BAlert* alert = new BAlert(B_TRANSLATE("About Tolmach"B_UTF8_ELLIPSIS),
		  str.String(), B_TRANSLATE("OK"));

  BTextView *view = alert->TextView();
  BFont font;
  view->SetStylable(true);
  view->GetFont(&font);
  font.SetSize(font.Size() * 1.5);
  font.SetFace(B_BOLD_FACE);
  view->SetFontAndColor(0, headerLen, &font);

  alert->Go();
}

void
TolmachApplication::ShowAlert(const char* title, const char *message,
                   alert_type type /*= B_INFO_ALERT*/, status_t status /*= B_OK*/)
{
  ShowAlert(BString(title), BString(message), type, status);
}

void TolmachApplication::ShowAlert(const BString &strTitle, const BString &strMessage,
                   alert_type type /*= B_INFO_ALERT*/, status_t status /*= B_OK*/)
{
  BString str(strMessage);
  if(status != B_OK){
    str << B_TRANSLATE("\n\nstatus is: (") << status << ") " << strerror(status);
  }
  BAlert* alert = new BAlert(strTitle.String(), str.String(),
                              B_TRANSLATE("OK"), 0, 0, B_WIDTH_AS_USUAL, type);
  alert->Go(); 
}

void TolmachApplication::UpdateMIMETypes()
{
  BMimeType mime;
  bool valid = false;
  mime.SetType(cszDictionaryMimeType);
  if(mime.IsInstalled()){
    BMessage  info;
    if(mime.GetAttrInfo(&info) == B_NO_ERROR){
      int32 index = 0;
      const char* str;
      while(info.FindString("attr:name", index++, &str) == B_NO_ERROR){
        if(!strcmp(str, cszAttrDescr)){
          valid = TRUE;
          break;
        }
      }
      if(!valid)
        mime.Delete();
    }
  }
  if(!valid){
	BBitmap	largeIcon(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_COLOR_8_BIT);
	BBitmap	miniIcon(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_COLOR_8_BIT);
    BMessage  msgExts, msg;
    mime.Install();
    
    BResources *pResources = AppResources();
    size_t largeIconSize = 0, miniIconSize = 0;
    const void *pLargeIconData = pResources->LoadResource('ICON', cszLargeIconResName, &largeIconSize); 
    if(pLargeIconData)
      largeIcon.SetBits(pLargeIconData, largeIcon.BitsLength(), 0, B_COLOR_8_BIT);
    const void *pMiniIconData = pResources->LoadResource('MICN', cszMiniIconResName, &miniIconSize); 
    if(pMiniIconData)
      miniIcon.SetBits(pMiniIconData, miniIcon.BitsLength(), 0, B_COLOR_8_BIT);
    mime.SetShortDescription("Polyglossum II base");
    mime.SetLongDescription("Polyglossum II dictionary database.");
    mime.SetIcon(&largeIcon, B_LARGE_ICON);
    mime.SetIcon(&miniIcon, B_MINI_ICON);
    mime.SetPreferredApp(cszApplicationSignature);
    
    msgExts.AddString("extensions", cszPGBExtBig);
    msgExts.AddString("extensions", cszPGBExt);
    mime.SetFileExtensions(&msgExts);

		// add relevant person fields to meta-mime type
    msg.AddString("attr:public_name", "Description"); 
    msg.AddString("attr:name", cszAttrDescr); 
    msg.AddInt32("attr:type", B_STRING_TYPE); 
    msg.AddBool("attr:viewable", true); 
    msg.AddBool("attr:editable", true); 
    msg.AddInt32("attr:width", 240); 
    msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
    msg.AddBool("attr:extra", false); 

    msg.AddString("attr:public_name", "Org.Langugae"); 
    msg.AddString("attr:name", cszAttrOLang); 
    msg.AddInt32("attr:type", B_STRING_TYPE); 
    msg.AddBool("attr:viewable", true); 
    msg.AddBool("attr:editable", true); 
    msg.AddInt32("attr:width", 70); 
    msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
    msg.AddBool("attr:extra", false); 

    msg.AddString("attr:public_name", "Dest.Language"); 
    msg.AddString("attr:name", cszAttrDLang); 
    msg.AddInt32("attr:type", B_STRING_TYPE); 
    msg.AddBool("attr:viewable", true); 
    msg.AddBool("attr:editable", true); 
    msg.AddInt32("attr:width", 70); 
    msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
    msg.AddBool("attr:extra", false); 

    mime.SetAttrInfo(&msg);
  }
}


status_t
TolmachApplication::GetDictAt(DictDescription *pddescr, int idx) const
{
  if(!pddescr || idx < 0 || idx >= m_dicts.size())
    return B_ERROR;
  *pddescr = m_dicts[idx];
  return B_OK;
}

int
TolmachApplication::DictCount() const
{
  return m_dicts.size();
}

void
TolmachApplication::LoadDictList()
{
  BPath pathDicts;
  find_directory(B_SYSTEM_DATA_DIRECTORY, &pathDicts);
  pathDicts.Append(cszDictionariesDir);
  BDirectory dir(pathDicts.Path());
  BEntry entry;
  if(B_OK != dir.InitCheck()){
    BString strMessage(B_TRANSLATE("Dictionaries directory '%dictsDir%' doesn't exists."));
	strMessage.ReplaceFirst("%dictsDir%", pathDicts.Path());
	theApp.ShowAlert(B_TRANSLATE("Error"), strMessage.String(), B_STOP_ALERT);
    return;
  }
  while(B_ENTRY_NOT_FOUND != dir.GetNextEntry(&entry)){
	LoadDictFile(entry);
  }
}

int
TolmachApplication::LoadDictFile(BEntry& entry)
{
  char name[B_FILE_NAME_LENGTH]; 
  entry.GetName(name);
  if(0 == strcasecmp(name + strlen(name) - strlen(cszPGBExt), cszPGBExt)){
    DictDescription dict;
    BPath path;
    entry.GetPath(&path);
    dict.path = path.Path();
    dict.name = path.Leaf(); //init for non-attributed bases
    BNode node(&entry);
    char attr_name[B_ATTR_NAME_LENGTH];
    while(B_ENTRY_NOT_FOUND != node.GetNextAttrName(attr_name)){
      attr_info ai;
      node.GetAttrInfo(attr_name, &ai);
      BString *pstr = 0;
      if(0 == strcmp(attr_name, cszAttrDescr)){
        pstr = &dict.name;
      }else
      if(0 == strcmp(attr_name, cszAttrOLang)){
        pstr = &dict.langs[eOLng];
      }else
      if(0 == strcmp(attr_name, cszAttrDLang)){
        pstr = &dict.langs[eDLng];
      }
      if(pstr){
        char *pbuf = new char[ai.size + 1];
        node.ReadAttr(attr_name, ai.type, 0, pbuf, ai.size);
        pbuf[ai.size] = 0;
        pstr->SetTo(pbuf);
      }
    }
    m_dicts.push_back(dict);
	return m_dicts.size() - 1;
  }

  return -1;
}

