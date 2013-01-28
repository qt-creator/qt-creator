/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#ifndef ANALYZERCONSTANTS_H
#define ANALYZERCONSTANTS_H

#include <QtGlobal>

namespace Analyzer {

// Special values for currently used start modes.
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

// Mode and its priority.
const char MODE_ANALYZE[] = "Mode.Analyze";
const int  P_MODE_ANALYZE = 76;

// Context.
const char C_ANALYZEMODE[] = "Analyzer.AnalyzeMode";

// Menu.
const char M_DEBUG_ANALYZER[] = "Analyzer.Menu.StartAnalyzer";
const char M_DEBUG_ANALYZER_QML_OPTIONS[] = "Analyzer.Menu.QMLOptions";

const char G_ANALYZER_CONTROL[] = "Menu.Group.Analyzer.Control";
const char G_ANALYZER_TOOLS[] = "Menu.Group.Analyzer.Tools";
const char G_ANALYZER_REMOTE_TOOLS[] = "Menu.Group.Analyzer.RemoteTools";
const char G_ANALYZER_OPTIONS[] = "Menu.Group.Analyzer.Options";

// Manager controls.
const char ANALYZER_CONTROL_START_ICON[] = ":/images/analyzer_start_small.png";
const char ANALYZER_CONTROL_STOP_ICON[] = ":/debugger/images/debugger_stop_small.png";

const char ANALYZERTASK_ID[] = "Analyzer.TaskId";

} // namespace Constants
} // namespace Analyzer

#endif // ANALYZERCONSTANTS_H
