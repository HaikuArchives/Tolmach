/*
 * PGBHandler.cpp
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
#include <ListView.h>
#include <StringView.h>
#include <TextView.h>
#include <TextControl.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <UTF8.h>
#include <Catalog.h>
#include <Locale.h>

#include "TolmachApp.h"
#include "TolmachWin.h"
#include "PGBHandler.h"
#include "Constants.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "PGBHandler"

#include "tables.inc" // encode tables

PGBHandler::PGBHandler(TolmachWindow *pOuterWin)
                         :m_pOuterWin(pOuterWin), m_nCurrent(-1),
						 m_bReverse(false), m_usNL(0), m_pPGBIndex(0),
                         m_nNumberWords(0), m_aTree(0), m_aTreeLett(0),
						 m_aWords(0), m_aTranslation(0)
{
}

PGBHandler::~PGBHandler()
{
  CloseDictionary();
}

int
PGBHandler::GetCurrent()
{
  return m_nCurrent;
}

bool
PGBHandler::GetReverse()
{
  return m_bReverse;
}
   

void
PGBHandler::SetCurrent(int idx, bool bReverse)
{
  if(idx != m_nCurrent || bReverse != m_bReverse){
    CloseDictionary();
    if(idx > 0 || idx < theApp.DictCount()){
      m_bReverse = bReverse; 
      LoadDictionary(idx);
    }
  }
}

void
PGBHandler::SetStatusText(const char *message, int perc /*= -1*/)
{
  m_pOuterWin->Lock();
  if(perc != -1){
    BString str(message);
    str << " (" << perc << "%)";
    m_pOuterWin->m_pStatusView->SetText(str.String());
//    m_pOuterWin->m_pStatusView->Sync();
  }else{
    m_pOuterWin->m_pStatusView->SetText(message);
//    m_pOuterWin->m_pStatusView->Sync();
  }
  m_pOuterWin->Unlock();
}

void
PGBHandler::LoadDictionary(int idx)
{
  //idx should be already checked for vector bounds ...
  DictDescription dd;
  theApp.GetDictAt(&dd, idx);
  m_pOuterWin->SetTitle(DictNameString(idx, m_bReverse).String());
  SetStatusText(B_TRANSLATE("Initialize"B_UTF8_ELLIPSIS));
  try{
    status_t status = m_fileDict.SetTo(dd.path.Path(), B_READ_ONLY);
    if(B_OK != status)
      throw status;
    //read header variables 
    int size = sizeof(PGBHeader);
    if(size != (status = m_fileDict.Read(&m_Header, size)))
      throw status;
    
	m_usNL = (m_Header.m_sNumberLett + 1) / 2; // ??? optimize?
      // tree lett
    //m_aTreeLett.resize(m_usNL);
    m_aTreeLett = new unsigned char[m_usNL];
    m_fileDict.Seek(1053, SEEK_SET);
    for(int i=0; i < m_usNL; i++){
      int n_tmp = 0;
      size = sizeof(n_tmp);
      if(size != (status = m_fileDict.Read(&n_tmp, size)))
        throw status;
      m_aTreeLett[i] = (unsigned char)(n_tmp >> 24);
    }
      //tree
    //m_aTree.resize(2 * m_usNL - 1);
    m_aTree = new unsigned char[2 * m_usNL - 1];
    for(int i=0; i < 2 * m_usNL - 2; i++){
      short s_tmp = 0;
      size = sizeof(s_tmp);
      if(size != (status = m_fileDict.Read(&s_tmp, size)))
        throw status;
      m_aTree[i] = (unsigned char)(s_tmp >> 8);
    }
    m_aTree[2 * m_usNL - 2] = 2 * m_usNL - 1;
    
    LoadWords();
    
    m_pOuterWin->LockLooper();
    BString str_status(dd.name);
    str_status << " ( " << m_nNumberWords << B_TRANSLATE(" words )");
    //m_pOuterWin->m_pStatusView->SetText(str_status.String());
    SetStatusText(str_status.String());
    m_pOuterWin->UnlockLooper();
    m_nCurrent = idx;
  }catch(status_t status){
    //m_aTree.resize(1);
    m_aTree = new unsigned char[1];
    //m_aTreeLett.resize(1);
    m_aTreeLett = new unsigned char[1];
    //m_aWords.resize(1);
    m_aWords = new char[1];
    m_pPGBIndex = new PGBIndex(m_fileDict);

    BString str_status(dd.name);
    str_status << B_TRANSLATE(" failed to load. :(");
    //m_pOuterWin->m_pStatusView->SetText(str_status.String());
    SetStatusText(str_status.String());
    
    BString str(B_TRANSLATE("Dictionary loading failed."));
    str << B_TRANSLATE("\nDictionary:") << dd.name;
    theApp.ShowAlert(B_TRANSLATE("Error"), str.String(), B_STOP_ALERT, status);
  }  
}

void
PGBHandler::CloseDictionary()
{
  m_fileDict.Unset();
  UnloadWords();
  //m_aTree.clear();
  delete[] m_aTree;
  m_aTree = 0;
  //m_aTreeLett.clear();
  delete[] m_aTreeLett;
  m_aTreeLett = 0;
  m_nCurrent = -1;
  m_pOuterWin->Lock();
  m_pOuterWin->m_pTolmachView->m_pWordsList->DoForEach(FreeListItem);
  m_pOuterWin->m_pTolmachView->m_pWordsList->MakeEmpty();
  m_pOuterWin->Unlock();
}

BString
PGBHandler::DictNameString(int nDict, bool bReverse)
{
  DictDescription dd;
  theApp.GetDictAt(&dd, nDict);
  BString str(dd.name);
  str << " [ " <<
  ((bReverse) ? dd.langs[eDLng] : dd.langs[eOLng])
  << " -> " <<
  ((bReverse) ? dd.langs[eOLng] : dd.langs[eDLng])
  << " ] ";
  return str;
}

void
PGBHandler::LoadWords()
{
  SetStatusText(B_TRANSLATE("Initializing words. Please wait..."));
  int nWordsAddress = GetWordsAddress();
  int nIndexAddress = GetIndexAddress();
  int nIndexNumber = GetIndexNumber();
  
  m_pPGBIndex = new PGBIndex(m_fileDict, nIndexAddress, nIndexNumber);
  
  int size = nIndexAddress - nWordsAddress;
  //m_aWords.resize(size);
  m_aWords = new char[size];
  status_t status = B_OK;
  if(size == (status = m_fileDict.ReadAt(nWordsAddress, /*&m_aWords[0]*/m_aWords, size))){
    m_nNumberWords = m_pPGBIndex->GetNumberWords();
    BListView *pWordsView = m_pOuterWin->m_pTolmachView->m_pWordsList;
    int count = pWordsView->CountItems();
    if(count)
      pWordsView->RemoveItems(0, count);
    
    int j=-1;
    unsigned char firstInt;
    unsigned char lastInt;
    int translationInt;
    BString addStr(" ");
    BString bStr;
    int insrt;
    //m_aTranslation.resize(m_nWordsAddressOrg); //??? optimize?
    m_aTranslation = new unsigned char[m_Header.m_nWordsAddressOrg];

	m_fileDict.ReadAt(0, /*&m_aTranslation[0]*/m_aTranslation, m_Header.m_nWordsAddressOrg); // error check!!!
    
    BList List(m_nNumberWords);
    for (int i = 0; i < m_nNumberWords; i++){
      m_pPGBIndex->FindNext(j);
      firstInt = m_pPGBIndex->GetFirstLetter(j);
      lastInt = m_pPGBIndex->GetLastLetter(j);
      bStr.SetTo("");
      translationInt = m_pPGBIndex->GetTranslation(j);
      insrt = Word(translationInt, firstInt, lastInt, bStr, addStr);
      if (insrt >= 0){
        BString tmpString(bStr);
        tmpString.Insert(addStr, insrt);
        List.AddItem(new BStringItem(tmpString.String()));
      }else{
        List.AddItem(new BStringItem(bStr.String()));
      }
      if(0 == (i % (m_nNumberWords / 500 + 1)))
        SetStatusText(B_TRANSLATE("Loading words"), i*100/(m_nNumberWords));
    }
    SetStatusText(B_TRANSLATE("Fill in words list. Please wait ..."));
    m_pOuterWin->LockLooper();
    pWordsView->AddList(&List);
    m_pOuterWin->UnlockLooper();
  }else{
    //todo  
  }
}

bool
PGBHandler::FreeListItem(BListItem *item)
{
  BStringItem *sItem = dynamic_cast<BStringItem *>(item);
  if(sItem) delete sItem;
  return false;
}

void
PGBHandler::UnloadWords()
{
  delete m_pPGBIndex;
  m_pPGBIndex = 0;
  //m_aWords.clear();
  delete[] m_aWords;
  m_aWords = 0;
}

int
PGBHandler::GetIndexAddress()
{
  return m_bReverse ? m_Header.m_nIndexAddressDest : m_Header.m_nIndexAddressOrg;
}

int
PGBHandler::GetWordsAddress()
{
  return m_bReverse ? m_Header.m_nWordsAddressDest : m_Header.m_nWordsAddressOrg;
}
        
int
PGBHandler::GetIndexNumber()
{
  return m_bReverse ? m_Header.m_nIndexNumberDest : m_Header.m_nIndexNumberOrg;
}

BString
PGBHandler::AdditionalWordFast(int adress)
{
  unsigned char length;
  unsigned char number;
  unsigned char *mas;

  bool rus;
  rus=false;

  length = m_aTranslation[adress];
  mas = /*&m_aTranslation[adress+1]*/m_aTranslation + adress + 1;
  number = m_aTranslation[adress+length];

  BString b;
  b = "";
  bool type=false;
  //Прохождение по дереву и нахождение текста
  unsigned short t;
  int j=0;
  unsigned char masElement;
  masElement=*mas;

  if (number>0){
    for (int i=0; i<number; i++){
      //Установка в root
      t = m_aTree[ 2 * m_usNL - 2];
      do{
        if (j>=8){
          mas++;
          masElement=*(mas);
          j=0;
        }
        //Проход по дереву
        t = m_aTree[2 * (t - m_usNL) - 1 - (masElement&1)] ;
        j++;
        masElement>>=1;
      }while (t > m_usNL);
       //Проверка
      switch(m_aTreeLett[t-1]){
         // ,
        case 44: {i=number; break;}
        case 172: {i=number; break;}
        // .
        case 46: {i=number; break;}
        case 174: {i=number; break;}
        // ' '
        case 1:{i=number; break;}
        case 160:{i=number; break;}
        //:
        case 58: {i=number; break;}
        case 186: {i=number; break;}
        //;
        case 59: {i=number; break;}
        case 187: {i=number; break;}
        //Проверка рус.-анг.
        case 127:{
          t=m_aTree[2 * m_usNL - 2];
          do{
            if (j>=8){
              mas++;
              masElement=*(mas);
              j=0;
            }
            //Проход по дереву
            t=m_aTree[2*(t-m_usNL)-1-(masElement&1)] ;
            j++;
            masElement>>=1;
          }while (t>m_usNL);
          i++;
          type=true;
          break;
        }
        default:{
          if (type)
            b += engLetters[ m_aTreeLett[t-1]-1].pstr;
          break;
        }
      }
    }
  }else{
    for (int i=2; i<length-1;i++){
      if (mas[i]!=0x7F) {b += mas[i];}
      else {break;}
    }
  }
  return b;
}

int
PGBHandler::Word(const int& adress,
                              const unsigned char& firstLett,
                              const unsigned char& lastLett, BString &bStr/*char* b*/, BString& addStr)
{
  int result=-1;

  unsigned char length;
  unsigned char number;
  bool rus;
  rus=false;
  unsigned char *mas;
  void* pointerVoid;


  length = m_aTranslation[adress];
  mas = &m_aTranslation[adress+1];
  number = m_aTranslation[adress+length];
  pointerVoid = &m_aTranslation[adress+length+1];
  int addr;
  addr= * ((int*) pointerVoid);

  int resultTmp=0;

  //Прохождение по дереву и нахождение текста
  unsigned short t;
  int j = 0;
  unsigned char masElement;
  masElement=*mas;
  if (number>0){
    for (int i = 0; i < lastLett; i++){
      //Установка в root
      t = m_aTree[2 * m_usNL - 2];
      do{
        if (j >= 8){
          mas++;
          masElement = *(mas);
          j=0;
        }
        //Проход по дереву
        t = m_aTree[2 * ( t - m_usNL) - 1 - (masElement&1)] ;
        j++;
        masElement>>=1;
      }while (t > m_usNL);
       //Проверка
      switch(m_aTreeLett[t-1]){
         //Проверка ~
        case 254:{
           //переход к другой статье по адресу addr;
          if ((i>=firstLett-1)&&(result<0)){
            result=resultTmp;
            addStr=AdditionalWordFast(addr);
            break;
          }
        }
        default:{
          if (i>=firstLett-1){
            TransLetter *pointerLett;
            if (rus){
              pointerLett=rusLetters;
            }else{
              pointerLett=engLetters;
            }
            TransLetter *cur_tl = (pointerLett + m_aTreeLett[t-1]-1);
            bStr += cur_tl->pstr;
            resultTmp++;
          }
          break;
        }
        //Проверка рус.-анг.
        case 127:{
          t = m_aTree[ 2 * m_usNL - 2];
          do{
            if (j>=8){
              mas++;
              masElement=*(mas);
              j=0;
            }
            //Проход по дереву
            t = m_aTree[ 2 * (t - m_usNL) - 1 - (masElement&1)] ;
            j++;
            masElement>>=1;
          }while (t > m_usNL);
          rus=false;
          if (m_aTreeLett[t-1]==0x06) {rus=true;}
          if (m_aTreeLett[t-1]==0x07) {rus=true;}
          i++;
          break;
        }
      }
    }
  }else{
    //Мы имеем дело с простым переводом
    for (int j = firstLett; j <= lastLett; j++){
      bStr += mas[j-1];
    }
  }
  return result;
}

void
PGBHandler::WordHighlighted()
{
/*  int perc;
  perc=i*100/numberWords;
  data->progress->setValue(perc);
*/  
  int j = 0;
  int temp = 0;
  unsigned char num = 0;
  BString currentWord, s;

  TolmachView* pTolmachView = m_pOuterWin->m_pTolmachView;
  BTextView *pTransView = m_pOuterWin->m_pTolmachView->m_pTransView;
  pTransView->Delete(0, pTransView->TextLength());
  pTolmachView->ResetStyleArray();

  int cur_idx = m_pOuterWin->m_pTolmachView->m_pWordsList->CurrentSelection();
  for (int i=0; i < cur_idx; i++)
     m_pPGBIndex->FindNext(j);
     
  temp = m_pPGBIndex->GetWord(j);
  m_fileDict.Seek(m_pPGBIndex->GetTranslation(j), SEEK_SET);
  m_fileDict.Read(&num, sizeof(num));
  m_fileDict.Seek(num, SEEK_CUR);
  int bb;
  m_fileDict.Read(&bb, sizeof(bb));
  
  if (bb<0) {bb=-bb;}

  int numHight=0;
  int numBold=1;
  int bbTmp;
  int bbTmp2;
  bbTmp2=bb;
  for (int i = 0; i < GetIndexNumber(); i++)
    if (m_pPGBIndex->GetWord(i) == temp)
    {
      numHight++;
      m_fileDict.Seek(abs(m_pPGBIndex->GetTranslation(i)), SEEK_SET);
      m_fileDict.Read(&num, sizeof(num));
      m_fileDict.Seek(num, SEEK_CUR);
      m_fileDict.Read(&bbTmp, sizeof(bbTmp));
      if (bbTmp < 0) {bbTmp=-bbTmp;}
      if (bbTmp != bb)
      {
        bb=bbTmp;
        numBold++;
      }
    }
  bb=bbTmp2;

//  data->translation->setNumberHight(numHight);
//  data->translation->setNumberBold(numBold);

  s = AdditionalWord(bb);
  s += ":";
//  data->translation->insertLine(s);
  
  pTransView->Insert(s.String());
  pTransView->Insert("\n");

  pTolmachView->AppendStyleItem(true, 0, s.Length());
//ZZ
//  data->translation->boldMas[0]=0;
  
  int indexI=0;
  int lineI=1;
  int k=0;
  //Теперь собственно сам перевод
  BString sTmp;
  for (int i=0; i<GetIndexNumber(); i++){
    if (m_pPGBIndex->GetWord(i)==temp){
      //Проверка: не новое ли слово?
      m_fileDict.Seek(abs(m_pPGBIndex->GetTranslation(i)), SEEK_SET);
      m_fileDict.Read(&num, sizeof(num));
      m_fileDict.Seek(num, SEEK_CUR);
      m_fileDict.Read(&bbTmp, sizeof(bbTmp));
      if (bbTmp < 0) {bbTmp=-bbTmp;}
      if (bbTmp != bb){
        bb=bbTmp;
        sTmp = AdditionalWord(bb);
        sTmp += ":";
        //data->translation->insertLine(sTmp);
		pTolmachView->AppendStyleItem(true, pTransView->TextLength(), sTmp.Length());
        pTransView->Insert(sTmp.String());
        pTransView->Insert("\n");
        k++;
        //ZZ
        //data->translation->boldMas[k]=lineI;
//		pBoldStyle[k] = lineI;
        printf("boldMas[%d]:%d\n",k,lineI);

        lineI++;
      }
	  
	  int hiStart = m_pPGBIndex->GetFirstLetter(i);
	  int hiEnd = m_pPGBIndex->GetLastLetter(i);
	  int address = abs(m_pPGBIndex->GetTranslation(i));
      s = Translate(address, hiStart, hiEnd, indexI, lineI);
      
      //Вставка строки
      //data->translation->insertLine(s);
	  pTolmachView->AppendStyleItem(false, pTransView->TextLength() + hiStart, hiEnd - hiStart);
      pTransView->Insert(s.String());
      pTransView->Insert("\n");
      indexI++;
      lineI++;
    }
  }
  
  pTolmachView->ApplyStyleArray();
}

BString
PGBHandler::AdditionalWord(int adress)
{
  m_fileDict.Seek(adress, SEEK_SET);
  unsigned char length;
  unsigned char number;
  bool rus;
  rus=false;
  unsigned char *mas;
  m_fileDict.Read(&length, sizeof(length));
  mas = new unsigned char[length-1];
  m_fileDict.Read(mas, length - 1);
  m_fileDict.Read(&number, sizeof(number));

  int k=1;

  BString b;
  bool type=false;
  //Прохождение по дереву и нахождение текста
  unsigned short t;
  int j=0;

  if (number>0){
    for (int i=0; i<number; i++){
      //Установка в root
      t = m_aTree[2 * m_usNL - 2 ];
      do{
        k=j/8;
        //Проход по дереву
        t=m_aTree[2*(t-m_usNL)-1-(mas[k]&1)] ;
        j++;
        mas[k]>>=1;
      }while (t > m_usNL);
       //Проверка
      switch(m_aTreeLett[t-1]){
         // ,
        case 44: {i=number; break;}
        case 172: {i=number; break;}
        // .
        case 46: {i=number; break;}
        case 174: {i=number; break;}
        // ' '
        case 1:{i=number; break;}
        case 160:{i=number; break;}
        //:
        case 58: {i=number; break;}
        case 186: {i=number; break;}
        //;
        case 59: {i=number; break;}
        case 187: {i=number; break;}
        //Проверка рус.-анг.
        case 127:{
          t=m_aTree[2 * m_usNL - 2];
          do{
            k=j/8;
            //Проход по дереву
            t = m_aTree[2 * (t- m_usNL)-1-(mas[k]&1)] ;
            j++;
            mas[k]>>=1;
          }while (t > m_usNL);
          i++;
          type = true;
          break;
        }
        default:{
          if (type){
            b += engLetters[m_aTreeLett[t-1]-1].pstr;
          }
          break;
        }
      }
    }
  }else{
    for (int i=2; i<length-1;i++){
      if (mas[i]!=0x7F) { b += mas[i];}
      else {break;}
    }
  }
  delete[] mas;
  return b;
}

BString
PGBHandler::Translate(int adress, int& firstLett, int& lastLett,
                                    int indexI, int lineI)
{
  m_fileDict.Seek(adress, SEEK_SET);
  unsigned char length;
  unsigned char number;
  bool rus;
  rus=false;
  m_fileDict.Read(&length, sizeof(length));
  char *mas = new char[length-1];
  m_fileDict.Read(mas, length-1);

  m_fileDict.Read(&number, sizeof(number));
  int k=1;

  BString b;
  
  //Прохождение по дереву и нахождение текста
  unsigned short t;
  int j=0;
  int x1,x2;
  x1=0;//-1;
  x2=0;

  if (number > 0){
    for (int i = 0; i < number; i++){

      //Установка в root
      t=m_aTree[2*m_usNL-2];
      do {
        k=j/8;
        //Проход по дереву
        t=m_aTree[2*(t-m_usNL)-1-(mas[k]&1)] ;
        j++;
        mas[k]>>=1;
      } while (t>m_usNL);

      //Проверка рус./англ.
      if (m_aTreeLett[t-1]==0x7F) {
        t=m_aTree[2*m_usNL-2];
        do{
          k=j/8;
          t=m_aTree[2*(t-m_usNL)-1-(mas[k]&1)] ;
          j++;
          mas[k]>>=1;
        }while (t>m_usNL);
        rus=false;
        if (m_aTreeLett[t-1]==0x06) {rus=true;}
        if (m_aTreeLett[t-1]==0x07) {rus=true;}
        i++;

      } else {
        TransLetter& rLetter = (rus) ? rusLetters[m_aTreeLett[t-1]-1]
									 : engLetters[m_aTreeLett[t-1]-1];
        if(i < firstLett)
			x1 = b.Length();
        if(i <= lastLett)
			x2 = b.Length();
		b += rLetter.pstr;
      }
    }
    if(lastLett == number)
		x2 = b.Length();
  } else {
    //Мы имеем дело с простым переводом
    int i=0;
    for (int j = 0; j < length-1; j++){
       if (mas[i] == 127){
         i = i++;
       } else {
         if(i < firstLett)
		   x1 = b.Length();
         if(i <= lastLett)
		   x2 = b.Length();

         b += mas[i];
       }
       i++;
    }
    if(lastLett == length - 1)
		x2 = b.Length();
  }
  delete[] mas;

//  data->translation->hightMas[indexI].x1=x1;
  //data->translation->hightMas[indexI].x2=x2;
 // data->translation->hightMas[indexI].line=lineI;
  firstLett = x1; 
  lastLett = x2; 
  
  return b;
}

void
PGBHandler::Reverse()
{
  UnloadWords();
  m_bReverse = !m_bReverse;
  LoadWords();
  
  DictDescription dd;
  theApp.GetDictAt(&dd, m_nCurrent);
  BString str_status(dd.name);
  str_status << " ( " << m_nNumberWords << B_TRANSLATE(" words )");
  m_pOuterWin->m_pStatusView->SetText(str_status.String());
}

int
PGBHandler::Seek(const char* s, const unsigned int& pos)
{
  void* pointerVoid;
  char* tmpChar = /*&m_aWords[0]*/m_aWords;
  unsigned char num;
  char* word;
  int result=0;
  int compare=5;
  for(int i=0; i<m_nNumberWords; i++){
    pointerVoid=tmpChar;
    num=*((unsigned char*) pointerVoid);
    tmpChar++;
    word=tmpChar;
    if(num<pos){
      compare=strncmp(s,word, num);
      if (compare<0){
        result--;
        break;
      }
    }else{
      compare=strncmp(s,word, pos);
      if(compare<0){
        result--;
        break;
      }
      if (compare==0)
        break;
    }
    result++;
    tmpChar+=num;
  }
  if (result<0)
    result=0;
  if (result > m_nNumberWords-1)
    result=m_nNumberWords-1;
  return result;
}

void
PGBHandler::WordEditChanged()
{
  int j = 0;
  BString strDest = ConvertWordInput(m_pOuterWin->m_pTolmachView->m_pWordEdit->Text());
  unsigned int nLength = strDest.Length();//strlen(pText);
  if(nLength > 0){
    j = Seek(strDest.String(), nLength);
  }
  m_pOuterWin->m_pTolmachView->m_pWordsList->Select(j);
  m_pOuterWin->m_pTolmachView->m_pWordsList->ScrollToSelection();
}

BString
PGBHandler::ConvertWordInput(const char *pStr)
{
  BString sDest, sOrg(pStr);
  int32 srcLen = sOrg.Length();
  int32 destLen = srcLen;
  int32 state = 0;
  char *dest = sDest.LockBuffer(destLen + 1);
  convert_from_utf8(B_MS_DOS_866_CONVERSION, sOrg.String(), &srcLen,
                                           dest, &destLen, &state);
  for(int i = 0; i < destLen; i++){
    dest[i]=m_Header.m_aConvLett[(unsigned char)dest[i]];
  }
  sDest.UnlockBuffer(destLen);
  return sDest;
}

void
PGBHandler::WordListInvoked()
{
  //Показывает полную словарную статью
  int j=0;
  int temp=0;
  unsigned char num;
  BString s;

//  data->translation->clear();
  TolmachView* pTolmachView = m_pOuterWin->m_pTolmachView;
  BTextView *pTransView = m_pOuterWin->m_pTolmachView->m_pTransView;
  pTransView->Delete(0, pTransView->TextLength());
//  data->translation->setReadOnly(false);
  pTolmachView->ResetStyleArray();

//ZZ
//  data->translation->unloadHightMas();
//  data->translation->unloadBoldMas();

  int cur_idx = m_pOuterWin->m_pTolmachView->m_pWordsList->CurrentSelection();
  for (int i=0; i < cur_idx; i++)
     m_pPGBIndex->FindNext(j);

  temp = m_pPGBIndex->GetWord(j);
  m_fileDict.Seek(m_pPGBIndex->GetTranslation(j), SEEK_SET);
  m_fileDict.Read(&num, sizeof(num));
  m_fileDict.Seek(num, SEEK_CUR);
  int bb;
  m_fileDict.Read(&bb, sizeof(bb));
  
  if (bb<0) {bb=-bb;}

  int numBold=1;
  int bbTmp;
  for (int i=0; i < GetIndexNumber(); i++)
    if (m_pPGBIndex->GetWord(i)==temp){
      m_fileDict.Seek(abs(m_pPGBIndex->GetTranslation(i)), SEEK_SET);
      m_fileDict.Read(&num, sizeof(num));
      m_fileDict.Seek(num, SEEK_CUR);
      m_fileDict.Read(&bbTmp, sizeof(bbTmp));
    
      if (bbTmp<0) {bbTmp=-bbTmp;}
      if (bbTmp!=bb)
      {
        bb=bbTmp;
        numBold++;
      }
    }
  bb=-1;
  //Формирования массива с основными адресами
  int* addrMas=new int [numBold];
  int intTmp;
  intTmp=0;

  for (int i=0; i < GetIndexNumber(); i++)
    if (m_pPGBIndex->GetWord(i)==temp){
      m_fileDict.Seek(abs(m_pPGBIndex->GetTranslation(i)), SEEK_SET);
      m_fileDict.Read(&num, sizeof(num));
      m_fileDict.Seek(num, SEEK_CUR);
      m_fileDict.Read(&bbTmp, sizeof(bbTmp));

      if (bbTmp<0) {bbTmp=-bbTmp;}
      if (bbTmp!=bb)
      {
        bb=bbTmp;
        addrMas[intTmp]=bb;
        intTmp++;
      }
    }
  bb=-1;

//ZZ
//  data->translation->setNumberBold(numBold);
//  data->translation->setNumberHight(1);

  int lineI=0;
  int k=0;
  //Теперь собственно сам полный перевод
  BString sTmp;
  m_fileDict.Seek(m_Header.m_nTransAddr, SEEK_SET);
  int trAdd;
  int savePosition;

  while (m_fileDict.Position() < m_Header.m_nWordsAddressOrg - 1){
      trAdd = m_fileDict.Position();
      m_fileDict.Read(&num, sizeof(num));
      m_fileDict.Seek(num, SEEK_CUR);
      m_fileDict.Read(&bbTmp, sizeof(bbTmp));
      if (bbTmp<0) {bbTmp=-bbTmp;}
      for (int i=0; i<numBold; i++)
      {
        if (bbTmp==addrMas[i])
        {
          savePosition=m_fileDict.Position();
          if (bbTmp!=bb)
          {
            bb=bbTmp;
            sTmp=AdditionalWord(bb);
            sTmp += ":";
            //data->translation->insertLine(sTmp);
            //data->translation->boldMas[k]=lineI; //ZZ
			pTolmachView->AppendStyleItem(true, pTransView->TextLength(), sTmp.Length());
            pTransView->Insert(sTmp.String());
            pTransView->Insert("\n");
            k++;
            lineI++;
          }
          s = Translate( trAdd );
          //data->translation->insertLine(s);
          pTransView->Insert(s.String());
          pTransView->Insert("\n");
          lineI++;
          m_fileDict.Seek(savePosition, SEEK_SET);
        }
      }
  }
  delete[] addrMas;
//  data->translation->setReadOnly(true);
  pTolmachView->ApplyStyleArray();
}

BString
PGBHandler::Translate(int adress)
{
  m_fileDict.Seek(adress, SEEK_SET);
//  fseek(dictionary,adress,0);
  unsigned char length;
  unsigned char number;
  bool rus;
  rus=false;
  m_fileDict.Read(&length, sizeof(length));
  unsigned char *mas = new unsigned char[length-1];
  m_fileDict.Read(mas, length-1);
  m_fileDict.Read(&number, sizeof(number));
  
  int k=1;

  BString b;
  //Прохождение по дереву и нахождение текста
  unsigned short t;
  int j=0;

  if(number>0){
    for (int i=0; i<number; i++){
      //Установка в root
      t=m_aTree[2*m_usNL-2];
      do{
        k=j/8;
        //Проход по дереву
        t=m_aTree[2*(t-m_usNL)-1-(mas[k]&1)] ;
        j++;
        mas[k]>>=1;
      }while (t>m_usNL);
      //Проверка рус./англ.
      if (m_aTreeLett[t-1]==127){
        t=m_aTree[2*m_usNL-2];
        do{
          k=j/8;
          t=m_aTree[2*(t-m_usNL)-1-(mas[k]&1)] ;
          j++;
          mas[k]>>=1;
        }while (t>m_usNL);
        rus=false;
        if (engLetters[m_aTreeLett[t-1]-1].pstr[0] == 'A') {rus=true;}
        if (engLetters[m_aTreeLett[t-1]-1].pstr[0] == 'B') {rus=true;}
        i++;
      }else{
        if (rus){
          b += rusLetters[m_aTreeLett[t-1]-1].pstr;
        }else{
          b += engLetters[m_aTreeLett[t-1]-1].pstr;
        }
      }
    }
  }else{
   //Мы имеем дело с простым переводом
    int i=0;
    for (int j=0; j<length-1; j++){
       if (mas[i]==127){
         i=i++;
       }else{
         b += mas[i];
       }
       i++;
    }
  }
  delete[] mas;

  return b;
}

