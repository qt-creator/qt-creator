/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef ANALYZERCONSTANTS_H
#define ANALYZERCONSTANTS_H

#include <QtCore/QtGlobal>

namespace Analyzer {

// Special values for currently used modes.
// Their meaning is interpreted by the individual tools.
// FIXME: The plan is to remove this entirely from the
// public interface and let the tools handle that internally.

enum StartMode
{
    StartLocal = -1,
    StartRemote = -2,
    StartQml = -3
};

namespace Constants {

// modes and their priorities
const char * const MODE_ANALYZE   = "Mode.Analyze";
const int          P_MODE_ANALYZE = 76;

// context
const char * const C_ANALYZEMODE = "Analyzer.AnalyzeMode";

// menu
const char * const M_DEBUG_ANALYZER = "Analyzer.Menu.StartAnalyzer";

const char * const START = "Analyzer.Start";
const char * const STARTREMOTE = "Analyzer.StartRemote";
const char * const STOP = "Analyzer.Stop";

const char * const G_ANALYZER_CONTROL = "Menu.Group.Analyzer.Control";
const char * const G_ANALYZER_TOOLS = "Menu.Group.Analyzer.Tools";
const char * const G_ANALYZER_REMOTE_TOOLS = "Menu.Group.Analyzer.RemoteTools";

// options dialog
const char * const ANALYZER_SETTINGS_CATEGORY = "T.Analyzer";
const char * const ANALYZER_SETTINGS_TR_CATEGORY =
    QT_TRANSLATE_NOOP("Analyzer", "Analyzer");
const char * const ANALYZER_SETTINGS_CATEGORY_ICON =
    ":/images/analyzer_category.png";

// manager controls
const char * const ANALYZER_CONTROL_START_ICON =
    ":/images/analyzer_start_small.png";
const char * const ANALYZER_CONTROL_STOP_ICON =
    ":/debugger/images/debugger_stop_small.png";

const char * const ANALYZERTASK_ID = "Analyzer.TaskId";

} // namespace Constants
} // namespace Analyzer

#endif // ANALYZERCONSTANTS_H
