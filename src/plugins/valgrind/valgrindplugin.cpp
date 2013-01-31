/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindplugin.h"

#include "callgrindtool.h"
#include "memchecktool.h"

#include <analyzerbase/analyzerconstants.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerrunconfigwidget.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/startremotedialog.h>

#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/hostosinfo.h>
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
    if (!Utils::HostOsInfo::isWindowsHost())
        modes.append(StartMode(StartLocal));
    modes.append(StartMode(StartRemote));

    AnalyzerManager::addTool(new MemcheckTool(this), modes);
    AnalyzerManager::addTool(new CallgrindTool(this), modes);

    return true;
}

} // namespace Internal
} // namespace Valgrind


Q_EXPORT_PLUGIN(Valgrind::Internal::ValgrindPlugin)
