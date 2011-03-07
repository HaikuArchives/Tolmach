/*
 * Constants.h
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

#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

const int32 MSG_CMD_DICT_NUM   = 'dICT';
const int32 MSG_CMD_DICT_PREFS = 'dPRF';
const int32 MSG_CMD_DICT_NONE = 'dNNE';
const int32 MSG_CMD_QUIT_REQUESTED = 'QURC';
const int32 MSG_EDIT_CHANGE = 'ECHG';
const int32 MSG_LIST_CHANGE = 'LTHG';
const int32 MSG_LIST_INVOKE = 'LTIV';
const int32 MSG_CMD_LOAD_CURRENT_DICT = 'LCDT';
const int32 MSG_CMD_UPDATE_DICT_MENU = 'UPDM';

const rgb_color BKG_GREY = { 216, 216, 216, 0 };

const char *const cszApplicationSignature = "application/x-vnd.Zyozik-Tolmach";
const char *const cszDictionaryMimeType = "application/x-vnd.ETS-PolyglossumII-base";
const char *const cszAttrDescr = "TDict:Descr";
const char *const cszAttrOLang = "TDict:OLang";
const char *const cszAttrDLang = "TDict:DLang";
const char *const cszPGBExt       = "pgb";
const char *const cszPGBExtBig    = "PGB";

const char *const cszLargeIconResName = "BEOS:L:application/x-Polyglossum-II-base";
const char *const cszMiniIconResName = "BEOS:M:application/x-Polyglossum-II-base";

const char *const cszLoadCurDictMsgItem = "cur_dict";
const char *const cszDictionariesDir    = "Tolmach";

#endif //_CONSTANTS_H_
