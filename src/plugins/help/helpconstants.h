// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>
#include <QLatin1String>

namespace Help::Constants {

const QLatin1String ListSeparator("|");
const QLatin1String AboutBlank("about:blank");

inline constexpr int  P_MODE_HELP    = 70;
const char ID_MODE_HELP  [] = "Help";

const char C_MODE_HELP   [] = "Help Mode";
inline constexpr char C_HELP_SIDEBAR[] = "Help Sidebar";
inline constexpr char C_HELP_EXTERNAL[] = "Help.ExternalWindow";

inline constexpr char CONTEXT_HELP[] = "Help.Context";
inline constexpr char HELP_HOME[] = "Help.Home";
inline constexpr char HELP_PREVIOUS[] = "Help.Previous";
inline constexpr char HELP_NEXT[] = "Help.Next";
inline constexpr char HELP_ADDBOOKMARK[] = "Help.AddBookmark";
inline constexpr char HELP_OPENONLINE[] = "Help.OpenOnline";
inline constexpr char HELP_INDEX[] = "Help.Index";
inline constexpr char HELP_CONTENTS[] = "Help.Contents";
inline constexpr char HELP_SEARCH[] = "Help.Search";
inline constexpr char HELP_BOOKMARKS[] = "Help.Bookmarks";
inline constexpr char HELP_OPENPAGES[] = "Help.OpenPages";

inline constexpr char SB_INDEX[] = QT_TRANSLATE_NOOP("QtC::Help", "Index");
inline constexpr char SB_CONTENTS[] = QT_TRANSLATE_NOOP("QtC::Help", "Contents");
inline constexpr char SB_BOOKMARKS[] = QT_TRANSLATE_NOOP("QtC::Help", "Bookmarks");
inline constexpr char SB_OPENPAGES[] = QT_TRANSLATE_NOOP("QtC::Help", "Open Pages");
inline constexpr char SB_SEARCH[] = QT_TRANSLATE_NOOP("QtC::Help", "Search");

inline constexpr char TR_OPEN_LINK_AS_NEW_PAGE[] = QT_TRANSLATE_NOOP("QtC::Help", "Open Link as New Page");
inline constexpr char TR_OPEN_LINK_IN_WINDOW[] = QT_TRANSLATE_NOOP("QtC::Help", "Open Link in Window");

} // Help::Constants
