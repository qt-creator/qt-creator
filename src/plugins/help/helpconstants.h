// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>
#include <QLatin1String>

namespace Help::Constants {

const QLatin1String ListSeparator("|");
const QLatin1String AboutBlank("about:blank");

const int  P_MODE_HELP    = 70;
const char ID_MODE_HELP  [] = "Help";
const char HELP_CATEGORY[] = "H.Help";

const char C_MODE_HELP   [] = "Help Mode";
const char C_HELP_SIDEBAR[] = "Help Sidebar";
const char C_HELP_EXTERNAL[] = "Help.ExternalWindow";

const char CONTEXT_HELP[] = "Help.Context";
const char HELP_HOME[] = "Help.Home";
const char HELP_PREVIOUS[] = "Help.Previous";
const char HELP_NEXT[] = "Help.Next";
const char HELP_ADDBOOKMARK[] = "Help.AddBookmark";
const char HELP_OPENONLINE[] = "Help.OpenOnline";
const char HELP_INDEX[] = "Help.Index";
const char HELP_CONTENTS[] = "Help.Contents";
const char HELP_SEARCH[] = "Help.Search";
const char HELP_BOOKMARKS[] = "Help.Bookmarks";
const char HELP_OPENPAGES[] = "Help.OpenPages";

const char SB_INDEX[] = QT_TRANSLATE_NOOP("QtC::Help", "Index");
const char SB_CONTENTS[] = QT_TRANSLATE_NOOP("QtC::Help", "Contents");
const char SB_BOOKMARKS[] = QT_TRANSLATE_NOOP("QtC::Help", "Bookmarks");
const char SB_OPENPAGES[] = QT_TRANSLATE_NOOP("QtC::Help", "Open Pages");
const char SB_SEARCH[] = QT_TRANSLATE_NOOP("QtC::Help", "Search");

const char TR_OPEN_LINK_AS_NEW_PAGE[] = QT_TRANSLATE_NOOP("QtC::Help", "Open Link as New Page");
const char TR_OPEN_LINK_IN_WINDOW[] = QT_TRANSLATE_NOOP("QtC::Help", "Open Link in Window");

} // Help::Constants
