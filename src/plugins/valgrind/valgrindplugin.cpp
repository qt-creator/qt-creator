/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindplugin.h"

#include "callgrindtool.h"
#include "memchecktool.h"

#include <analyzerbase/analyzerconstants.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerrunconfigwidget.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/startremotedialog.h>

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QStringList>
#include <QtPlugin>
#include <QAction>

using namespace Analyzer;
using namespace ProjectExplorer;


namespace Valgrind {
namespace Internal {

static void startRemoteTool(IAnalyzerTool *tool)
{
    Q_UNUSED(tool);
    StartRemoteDialog dlg;
    if (dlg.exec() != QDialog::Accepted)
        return;

    AnalyzerStartParameters sp;
    sp.toolId = tool->id();
    sp.startMode = StartRemote;
    sp.connParams = dlg.sshParams();
    sp.debuggee = dlg.executable();
    sp.debuggeeArgs = dlg.arguments();
    sp.displayName = dlg.executable();
    sp.workingDirectory = dlg.workingDirectory();

    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, 0);
    //m_currentRunControl = rc;
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));

    ProjectExplorerPlugin::instance()->startRunControl(rc, tool->runMode());
}

void ValgrindPlugin::startValgrindTool(IAnalyzerTool *tool, StartMode mode)
{
    if (mode == StartLocal)
        AnalyzerManager::startLocalTool(tool);
    if (mode == StartRemote)
        startRemoteTool(tool);
}

bool ValgrindPlugin::initialize(const QStringList &, QString *)
{
    StartModes modes;
#ifndef Q_OS_WIN
    modes.append(StartMode(StartLocal));
#endif
    modes.append(StartMode(StartRemote));

    AnalyzerManager::addTool(new MemcheckTool(this), modes);
    AnalyzerManager::addTool(new CallgrindTool(this), modes);

    return true;
}

} // namespace Internal
} // namespace Valgrind


Q_EXPORT_PLUGIN(Valgrind::Internal::ValgrindPlugin)
