/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef HELPCONSTANTS_H
#define HELPCONSTANTS_H

#include <QtGlobal>
#include <QLatin1String>

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

const int  P_MODE_HELP    = 70;
const char ID_MODE_HELP  [] = "Help";
const char HELP_CATEGORY[] = "H.Help";
const char HELP_CATEGORY_ICON[] = ":/core/images/category_help.png";
const char HELP_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Help", "Help");

const char C_MODE_HELP   [] = "Help Mode";
const char C_HELP_SIDEBAR[] = "Help Sidebar";

} // Constants
} // Help

#endif // HELPCONSTANTS_H
