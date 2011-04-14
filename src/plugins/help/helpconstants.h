/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef HELPCONSTANTS_H
#define HELPCONSTANTS_H

#include <QtCore/QtGlobal>
#include <QtCore/QLatin1String>

namespace Help {
    namespace Constants {

enum {
    ShowHomePage = 0,
    ShowBlankPage = 1,
    ShowLastPages = 2
};

enum {
    SideBySideIfPossible = 0,
    SideBySideAlways = 1,
    FullHelpAlways = 2,
    ExternalHelpAlways = 3
};

static const QLatin1String ListSeparator("|");
static const QLatin1String DefaultZoomFactor("0.0");
static const QLatin1String AboutBlank("about:blank");
static const QLatin1String WeAddedFilterKey("UnfilteredFilterInserted");
static const QLatin1String PreviousFilterNameKey("UnfilteredFilterName");

const int          P_MODE_HELP    = 70;
const char * const ID_MODE_HELP   = "Help";
const char * const HELP_CATEGORY = "H.Help";
const char * const HELP_CATEGORY_ICON = ":/core/images/category_help.png";
const char * const HELP_TR_CATEGORY = QT_TRANSLATE_NOOP("Help", "Help");

const char * const C_MODE_HELP    = "Help Mode";
const char * const C_HELP_SIDEBAR = "Help Sidebar";

    }   // Constants
}   // Help

#endif // HELPCONSTANTS_H
