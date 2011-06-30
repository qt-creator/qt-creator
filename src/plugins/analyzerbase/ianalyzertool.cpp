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

#include "ianalyzertool.h"
#include "analyzermanager.h"

namespace Analyzer {

IAnalyzerTool::IAnalyzerTool(QObject *parent)
    : QObject(parent)
{}

QString IAnalyzerTool::modeString(ToolMode mode)
{
    switch (mode) {
        case IAnalyzerTool::DebugMode:
            return tr("Debug");
        case IAnalyzerTool::ReleaseMode:
            return tr("Release");
        case IAnalyzerTool::AnyMode:
            break;
    }
    return QString();
}

QByteArray IAnalyzerTool::defaultMenuGroup(StartMode mode)
{
    if (mode == StartRemote)
        return Analyzer::Constants::G_ANALYZER_REMOTE_TOOLS;
    return Analyzer::Constants::G_ANALYZER_TOOLS;
}

QByteArray IAnalyzerTool::defaultActionId(const QByteArray &id, StartMode mode)
{
    if (mode == StartRemote)
        return "Action." + id + ".RemoteStart." + QByteArray::number(mode);
    return "Action." + id + ".LocalStart." + QByteArray::number(mode);
}

QString IAnalyzerTool::defaultActionName(const QString &base, StartMode mode)
{
    if (mode == StartRemote)
        return base + tr(" (Remote)");
    return base;
}

void IAnalyzerTool::defaultStartTool(IAnalyzerTool *tool, StartMode mode)
{
    if (mode == StartLocal)
        AnalyzerManager::startLocalTool(tool, mode);
    if (mode == StartRemote)
        AnalyzerManager::startRemoteTool(tool, mode);
}

void IAnalyzerTool::initializeDockWidgets()
{
}

} // namespace Analyzer
