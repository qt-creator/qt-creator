/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef HELPCONSTANTS_H
#define HELPCONSTANTS_H

#include <QtGlobal>
#include <QLatin1String>

namespace Help {
namespace Constants {

static const QLatin1String ListSeparator("|");
static const QLatin1String AboutBlank("about:blank");
static const QLatin1String WeAddedFilterKey("UnfilteredFilterInserted");
static const QLatin1String PreviousFilterNameKey("UnfilteredFilterName");

const int  P_MODE_HELP    = 70;
const char ID_MODE_HELP  [] = "Help";
const char HELP_CATEGORY[] = "H.Help";
const char HELP_CATEGORY_ICON[] = ":/help/images/category_help.png";
const char HELP_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Help", "Help");

const char C_MODE_HELP   [] = "Help Mode";
const char C_HELP_SIDEBAR[] = "Help Sidebar";
const char C_HELP_EXTERNAL[] = "Help.ExternalWindow";

const char CONTEXT_HELP[] = "Help.Context";
const char HELP_HOME[] = "Help.Home";
const char HELP_PREVIOUS[] = "Help.Previous";
const char HELP_NEXT[] = "Help.Next";
const char HELP_ADDBOOKMARK[] = "Help.AddBookmark";
const char HELP_INDEX[] = "Help.Index";
const char HELP_CONTENTS[] = "Help.Contents";
const char HELP_SEARCH[] = "Help.Search";
const char HELP_BOOKMARKS[] = "Help.Bookmarks";
const char HELP_OPENPAGES[] = "Help.OpenPages";

static const char SB_INDEX[] = QT_TRANSLATE_NOOP("Help::Internal::HelpPlugin", "Index");
static const char SB_CONTENTS[] = QT_TRANSLATE_NOOP("Help::Internal::HelpPlugin", "Contents");
static const char SB_BOOKMARKS[] = QT_TRANSLATE_NOOP("Help::Internal::HelpPlugin", "Bookmarks");
static const char SB_OPENPAGES[] = QT_TRANSLATE_NOOP("Help::Internal::HelpPlugin", "Open Pages");
static const char SB_SEARCH[] = QT_TRANSLATE_NOOP("Help::Internal::HelpPlugin", "Search");

} // Constants
} // Help

#endif // HELPCONSTANTS_H
