/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "ianalyzertool.h"

#include "analyzermanager.h"
#include "analyzerruncontrol.h"

#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/qtcassert.h>
#include <utils/checkablemessagebox.h>

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSettings>

using namespace Core;
using namespace ProjectExplorer;

namespace Analyzer {

AnalyzerAction::AnalyzerAction(QObject *parent)
    : QAction(parent), m_toolMode(AnyMode)
{}

bool AnalyzerAction::isRunnable(QString *reason) const
{
    if (m_customToolStarter) // Something special. Pretend we can always run it.
        return true;

    return ProjectExplorerPlugin::canRun(SessionManager::startupProject(), m_runMode, reason);
}

static bool buildTypeAccepted(QFlags<ToolMode> toolMode, BuildConfiguration::BuildType buildType)
{
    if (buildType == BuildConfiguration::Unknown)
        return true;
    if (buildType == BuildConfiguration::Debug && (toolMode & DebugMode))
        return true;
    if (buildType == BuildConfiguration::Release && (toolMode & ReleaseMode))
        return true;
    if (buildType == BuildConfiguration::Profile && (toolMode & ProfileMode))
        return true;
    return false;
}

void AnalyzerAction::startTool()
{
    // Make sure mode is shown.
    AnalyzerManager::showMode();

    TaskHub::clearTasks(Constants::ANALYZERTASK_ID);
    AnalyzerManager::showPermanentStatusMessage(toolId(), QString());

    if (m_toolPreparer && !m_toolPreparer())
        return;

    // ### not sure if we're supposed to check if the RunConFiguration isEnabled
    Project *pro = SessionManager::startupProject();
    RunConfiguration *rc = 0;
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;
    if (pro) {
        if (const Target *target = pro->activeTarget()) {
            // Build configuration is 0 for QML projects.
            if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
                buildType = buildConfig->buildType();
            rc = target->activeRunConfiguration();
        }
    }

    // Custom start.
    if (m_customToolStarter) {
        if (rc) {
            m_customToolStarter(rc);
        } else {
            const QString toolName = text();
            QMessageBox *errorDialog = new QMessageBox(ICore::mainWindow());
            errorDialog->setIcon(QMessageBox::Warning);
            errorDialog->setWindowTitle(toolName);
            errorDialog->setText(tr("Cannot start %1 without a project. Please open the project "
                                    "and try again.").arg(toolName));
            errorDialog->setStandardButtons(QMessageBox::Ok);
            errorDialog->setDefaultButton(QMessageBox::Ok);
            errorDialog->show();
        }
        return;
    }

    // Check the project for whether the build config is in the correct mode
    // if not, notify the user and urge him to use the correct mode.
    if (!buildTypeAccepted(m_toolMode, buildType)) {
        QString currentMode;
        switch (buildType) {
            case BuildConfiguration::Debug:
                currentMode = AnalyzerManager::tr("Debug");
                break;
            case BuildConfiguration::Profile:
                currentMode = AnalyzerManager::tr("Profile");
                break;
            case BuildConfiguration::Release:
                currentMode = AnalyzerManager::tr("Release");
                break;
            default:
                QTC_CHECK(false);
        }

        QString toolModeString;
        switch (m_toolMode) {
            case DebugMode:
                toolModeString = AnalyzerManager::tr("in Debug mode");
                break;
            case ProfileMode:
                toolModeString = AnalyzerManager::tr("in Profile mode");
                break;
            case ReleaseMode:
                toolModeString = AnalyzerManager::tr("in Release mode");
                break;
            case SymbolsMode:
                toolModeString = AnalyzerManager::tr("with debug symbols (Debug or Profile mode)");
                break;
            case OptimizedMode:
                toolModeString = AnalyzerManager::tr("on optimized code (Profile or Release mode)");
                break;
            default:
                QTC_CHECK(false);
        }
        const QString toolName = text(); // The action text is always the name of the tool
        const QString title = AnalyzerManager::tr("Run %1 in %2 Mode?").arg(toolName).arg(currentMode);
        const QString message = AnalyzerManager::tr("<html><head/><body><p>You are trying "
            "to run the tool \"%1\" on an application in %2 mode. "
            "The tool is designed to be used %3.</p><p>"
            "Run-time characteristics differ significantly between "
            "optimized and non-optimized binaries. Analytical "
            "findings for one mode may or may not be relevant for "
            "the other.</p><p>"
            "Running tools that need debug symbols on binaries that "
            "don't provide any may lead to missing function names "
            "or otherwise insufficient output.</p><p>"
            "Do you want to continue and run the tool in %2 mode?</p></body></html>")
                .arg(toolName).arg(currentMode).arg(toolModeString);
        if (Utils::CheckableMessageBox::doNotAskAgainQuestion(ICore::mainWindow(),
                title, message, ICore::settings(), QLatin1String("AnalyzerCorrectModeWarning"))
                    != QDialogButtonBox::Yes)
            return;
    }

    ProjectExplorerPlugin::runStartupProject(m_runMode);
}

} // namespace Analyzer
