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
#include "analyzerruncontrol.h"
#include "startremotedialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
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

Id IAnalyzerTool::menuGroup(StartMode mode) const
{
    if (mode == StartRemote)
        return Constants::G_ANALYZER_REMOTE_TOOLS;
    return Constants::G_ANALYZER_TOOLS;
}

QString IAnalyzerTool::actionName(StartMode mode) const
{
    QString base = displayName();
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

static void startLocalTool(IAnalyzerTool *tool)
{
    // Make sure mode is shown.
    AnalyzerManager::showMode();

    ProjectExplorerPlugin *pe = ProjectExplorerPlugin::instance();

    // ### not sure if we're supposed to check if the RunConFiguration isEnabled
    Project *pro = pe->startupProject();
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
    if (!buildTypeAccepted(tool->toolMode(), buildType)) {
        const QString currentMode = buildType == BuildConfiguration::Debug
                ? AnalyzerManager::tr("Debug")
                : AnalyzerManager::tr("Release");

        QSettings *settings = ICore::settings();
        const QString configKey = QLatin1String("Analyzer.AnalyzeCorrectMode");
        int ret;
        if (settings->contains(configKey)) {
            ret = settings->value(configKey, QDialog::Accepted).toInt();
        } else {
            QString toolModeString;
            switch (tool->toolMode()) {
                case IAnalyzerTool::DebugMode:
                    toolModeString = AnalyzerManager::tr("Debug");
                    break;
                case IAnalyzerTool::ReleaseMode:
                    toolModeString = AnalyzerManager::tr("Release");
                    break;
                default:
                    QTC_CHECK(false);
            }
            const QString toolName = tool->displayName();
            const QString title = AnalyzerManager::tr("Run %1 in %2 Mode?").arg(toolName).arg(currentMode);
            const QString message = AnalyzerManager::tr("<html><head/><body><p>You are trying "
                "to run the tool \"%1\" on an application in %2 mode. "
                "The tool is designed to be used in %3 mode.</p><p>"
                "Debug and Release mode run-time characteristics differ "
                "significantly, analytical findings for one mode may or "
                "may not be relevant for the other.</p><p>"
                "Do you want to continue and run the tool in %2 mode?</p></body></html>")
                    .arg(toolName).arg(currentMode).arg(toolModeString);
            const QString checkBoxText = AnalyzerManager::tr("&Do not ask again");
            bool checkBoxSetting = false;
            const QDialogButtonBox::StandardButton button =
                Utils::CheckableMessageBox::question(ICore::mainWindow(),
                    title, message, checkBoxText,
                    &checkBoxSetting, QDialogButtonBox::Yes|QDialogButtonBox::Cancel,
                    QDialogButtonBox::Cancel);
            ret = button == QDialogButtonBox::Yes ? QDialog::Accepted : QDialog::Rejected;

            if (checkBoxSetting && ret == QDialog::Accepted)
                settings->setValue(configKey, ret);
        }
        if (ret == QDialog::Rejected)
            return;
    }

    pe->runProject(pro, tool->runMode());
}

static void startRemoteTool(IAnalyzerTool *tool)
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

    AnalyzerRunControl *rc = tool->createRunControl(sp, 0);
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));

    ProjectExplorerPlugin::instance()->startRunControl(rc, tool->runMode());
}

void IAnalyzerTool::startTool(StartMode mode)
{
    if (mode == StartLocal)
        startLocalTool(this);
    if (mode == StartRemote)
        startRemoteTool(this);
}

} // namespace Analyzer
