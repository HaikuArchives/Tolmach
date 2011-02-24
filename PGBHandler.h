/*
 * DictHandler.h
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

#ifndef _PGBHANDLER_H_
#define _PGBHANDLER_H_

#include <String.h>
#include <File.h>
#include <cstdio>
#include <vector>

#include "TolmachView.h"

class TolmachWindow;

class PGBHandler
{
  class PGBIndex {
      //std::vector<unsigned char> m_aIndex;
      unsigned char *m_aIndex;
      int m_nElements;
      BFile &m_fileDict;
    public:
      PGBIndex(BFile &fileDict);
      PGBIndex(BFile &fileDict, int position, int number);
      ~PGBIndex();
        
      int GetWord(const int &pos);
      int GetTranslation(const int &pos);
      unsigned char GetFirstLetter(const int &pos);
      unsigned char GetLastLetter(const int &pos);
      int GetNumberWords();
      void FindNext(int &j);
  } *m_pPGBIndex;
      
    BFile m_fileDict;
    int m_nIndexAddressOrg;
    int m_nIndexAddressDest;
    int GetIndexAddress();
        
    int m_nWordsAddressOrg;
    int m_nWordsAddressDest;
    int GetWordsAddress();
        
    int m_nIndexNumberOrg;
    int m_nIndexNumberDest;
    int GetIndexNumber();
        
    int m_nTransAddr;
        
    short m_sNumberLett;
    int   m_nNumberWords;
    unsigned short m_usNL;
    //std::vector<unsigned char> m_aTree;
    unsigned char *m_aTree;
    //std::vector<unsigned char> m_aTreeLett;
    unsigned char *m_aTreeLett;
    //std::vector<char> m_aConvLett;
    char *m_aConvLett;
        
    //std::vector<char> m_aWords;
    char *m_aWords;
    //std::vector<unsigned char> m_aTranslation;
    unsigned char *m_aTranslation;
                
    bool m_bReverse;
  
    TolmachWindow *m_pOuterWin;
    int m_nCurrent;
    void LoadDictionary(int idx);
    void LoadWords();
    void UnloadWords();
    void CloseDictionary();
    int Word(const int& adress, const unsigned char& firstLett,
                                const unsigned char& lastLett,
                                BString &bStr/*char* b*/, BString& addStr);
    BString AdditionalWordFast(int adress);
    BString AdditionalWord(int adress);
    BString Translate(int adress, int firstLett, int lastLett,
                                  int indexI, int lineI);
    BString Translate(int adress);
    BString ConvertWordInput(const char*);
        
    int Seek(const char* s, const unsigned int& pos);
    
    void SetStatusText(const char *message, int perc = -1);
    BString DictNameString(int nDict, bool bReverse);
    
    static bool FreeListItem(BListItem *item);

  public:
    PGBHandler(TolmachWindow *pOuterWin);
    ~PGBHandler();
    void SetCurrent(int idx, bool bReverse);
    int  GetCurrent();
    void WordHighlighted();
    void WordEditChanged();
    void WordListInvoked();
    void Reverse();
    bool GetReverse();
};

#endif //_PGBHANDLER_H_
