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

#include "ianalyzertool.h"

#include "analyzermanager.h"
#include "analyzerruncontrol.h"
#include "startremotedialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>
#include <utils/checkablemessagebox.h>

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSettings>

using namespace Core;
using namespace ProjectExplorer;

namespace Analyzer {

IAnalyzerTool::IAnalyzerTool(QObject *parent)
    : QObject(parent)
{}

 /// Returns the run mode for this tool.
ProjectExplorer::RunMode IAnalyzerTool::runMode() const
{
    return m_runMode;
}

void IAnalyzerTool::setRunMode(ProjectExplorer::RunMode mode)
{
    m_runMode = mode;
}

IAnalyzerTool::ToolMode IAnalyzerTool::toolMode() const
{
    return m_toolMode;
}

void IAnalyzerTool::setToolMode(IAnalyzerTool::ToolMode mode)
{
    m_toolMode = mode;
}

AnalyzerAction::AnalyzerAction(QObject *parent)
    : QAction(parent)
{}

static bool buildTypeAccepted(IAnalyzerTool::ToolMode toolMode,
                       BuildConfiguration::BuildType buildType)
{
    if (toolMode == IAnalyzerTool::AnyMode)
        return true;
    if (buildType == BuildConfiguration::Unknown)
        return true;
    if (buildType == BuildConfiguration::Debug
            && toolMode == IAnalyzerTool::DebugMode)
        return true;
    if (buildType == BuildConfiguration::Release
            && toolMode == IAnalyzerTool::ReleaseMode)
        return true;
    return false;
}

void IAnalyzerTool::startLocalTool()
{
    // Make sure mode is shown.
    AnalyzerManager::showMode();

    // ### not sure if we're supposed to check if the RunConFiguration isEnabled
    Project *pro = SessionManager::startupProject();
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;
    if (pro) {
        if (const Target *target = pro->activeTarget()) {
            // Build configuration is 0 for QML projects.
            if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
                buildType = buildConfig->buildType();
        }
    }

    // Check the project for whether the build config is in the correct mode
    // if not, notify the user and urge him to use the correct mode.
    if (!buildTypeAccepted(toolMode(), buildType)) {
        const QString currentMode = buildType == BuildConfiguration::Debug
                ? AnalyzerManager::tr("Debug")
                : AnalyzerManager::tr("Release");

        QString toolModeString;
        switch (toolMode()) {
            case IAnalyzerTool::DebugMode:
                toolModeString = AnalyzerManager::tr("Debug");
                break;
            case IAnalyzerTool::ReleaseMode:
                toolModeString = AnalyzerManager::tr("Release");
                break;
            default:
                QTC_CHECK(false);
        }
        //const QString toolName = displayName();
        const QString toolName = AnalyzerManager::tr("Tool"); // FIXME
        const QString title = AnalyzerManager::tr("Run %1 in %2 Mode?").arg(toolName).arg(currentMode);
        const QString message = AnalyzerManager::tr("<html><head/><body><p>You are trying "
            "to run the tool \"%1\" on an application in %2 mode. "
            "The tool is designed to be used in %3 mode.</p><p>"
            "Debug and Release mode run-time characteristics differ "
            "significantly, analytical findings for one mode may or "
            "may not be relevant for the other.</p><p>"
            "Do you want to continue and run the tool in %2 mode?</p></body></html>")
                .arg(toolName).arg(currentMode).arg(toolModeString);
        if (Utils::CheckableMessageBox::doNotAskAgainQuestion(ICore::mainWindow(),
                title, message, ICore::settings(), QLatin1String("AnalyzerCorrectModeWarning"))
                    != QDialogButtonBox::Yes)
            return;
    }

    ProjectExplorerPlugin::instance()->runProject(pro, runMode());
}

void IAnalyzerTool::startRemoteTool()
{
    StartRemoteDialog dlg;
    if (dlg.exec() != QDialog::Accepted)
        return;

    AnalyzerStartParameters sp;
    sp.startMode = StartRemote;
    sp.connParams = dlg.sshParams();
    sp.debuggee = dlg.executable();
    sp.debuggeeArgs = dlg.arguments();
    sp.displayName = dlg.executable();
    sp.workingDirectory = dlg.workingDirectory();

    AnalyzerRunControl *rc = createRunControl(sp, 0);

    ProjectExplorerPlugin::startRunControl(rc, runMode());
}

} // namespace Analyzer
