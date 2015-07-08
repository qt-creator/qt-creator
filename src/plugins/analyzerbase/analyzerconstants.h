/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#ifndef ANALYZERCONSTANTS_H
#define ANALYZERCONSTANTS_H

#include <QtGlobal>

namespace Analyzer {
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
const char ANALYZER_CONTROL_STOP_ICON[] = ":/images/analyzer_stop_small.png";

const char ANALYZERTASK_ID[] = "Analyzer.TaskId";

} // namespace Constants
} // namespace Analyzer

#endif // ANALYZERCONSTANTS_H
