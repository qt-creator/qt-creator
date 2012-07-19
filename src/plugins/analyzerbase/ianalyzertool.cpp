/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

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
        return Constants::G_ANALYZER_REMOTE_TOOLS;
    return Constants::G_ANALYZER_TOOLS;
}

Id IAnalyzerTool::defaultActionId(const IAnalyzerTool *tool, StartMode mode)
{
    Id id = tool->id();
    switch (mode) {
    case StartLocal:
        return Id(QByteArray("Analyzer." + id.name() + ".Local").data());
    case StartRemote:
        return Id(QByteArray("Analyzer." + id.name() + ".Remote").data());
    case StartQml:
        return Id(QByteArray("Analyzer." + id.name() + ".Qml").data());
    }
    return Id();
}

QString IAnalyzerTool::defaultActionName(const IAnalyzerTool *tool, StartMode mode)
{
    QString base = tool->displayName();
    if (mode == StartRemote)
        return base + tr(" (Remote)");
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
