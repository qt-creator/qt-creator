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

#include "ianalyzertool.h"
#include "analyzermanager.h"

using namespace Core;

namespace Analyzer {

IAnalyzerTool::IAnalyzerTool(QObject *parent)
    : QObject(parent)
{}

Id IAnalyzerTool::defaultMenuGroup(StartMode mode)
{
    if (mode == StartRemote)
        return Id(Constants::G_ANALYZER_REMOTE_TOOLS);
    return Id(Constants::G_ANALYZER_TOOLS);
}

Id IAnalyzerTool::defaultActionId(const IAnalyzerTool *tool, StartMode mode)
{
    Id id = tool->id();
    switch (mode) {
    case StartLocal:
        return Id(QByteArray("Analyzer." + id.name() + ".Local"));
    case StartRemote:
        return Id(QByteArray("Analyzer." + id.name() + ".Remote"));
    case StartQml:
        return Id(QByteArray("Analyzer." + id.name() + ".Qml"));
    }
    return Id();
}

QString IAnalyzerTool::defaultActionName(const IAnalyzerTool *tool, StartMode mode)
{
    QString base = tool->displayName();
    if (mode == StartRemote)
        return base + tr(" (External)");
    return base;
}

AbstractAnalyzerSubConfig *IAnalyzerTool::createGlobalSettings()
{
    return 0;
}

AbstractAnalyzerSubConfig *IAnalyzerTool::createProjectSettings()
{
    return 0;
}

} // namespace Analyzer
