/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberryruncontrolfactory.h"
#include "blackberryrunconfiguration.h"
#include "blackberryruncontrol.h"
#include "blackberrydeployconfiguration.h"
#include "blackberrydebugsupport.h"
#include "blackberryqtversion.h"
#include "blackberrydeviceconnectionmanager.h"
#include "blackberryapplicationrunner.h"
#include "qnxutils.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerkitinformation.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <coreplugin/messagemanager.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryRunControlFactory::BlackBerryRunControlFactory(QObject *parent)
    : ProjectExplorer::IRunControlFactory(parent)
{
}

bool BlackBerryRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                                  ProjectExplorer::RunMode mode) const
{
    Q_UNUSED(mode);

    BlackBerryRunConfiguration *rc = qobject_cast<BlackBerryRunConfiguration *>(runConfiguration);
    if (!rc)
        return false;

    BlackBerryDeviceConfiguration::ConstPtr device = BlackBerryDeviceConfiguration::device(rc->target()->kit());
    if (!device)
        return false;

    // The device can only run the same application once, any subsequent runs will
    // not launch a second instance. Disable the Run button if the application is already
    // running on the device.
    if (m_activeRunControls.contains(rc->key())) {
        QPointer<ProjectExplorer::RunControl> activeRunControl = m_activeRunControls[rc->key()];
        if (activeRunControl && activeRunControl.data()->isRunning())
            return false;
        else
            m_activeRunControls.remove(rc->key());
    }

    BlackBerryDeployConfiguration *activeDeployConf = qobject_cast<BlackBerryDeployConfiguration *>(
                rc->target()->activeDeployConfiguration());
    return activeDeployConf != 0;
}

static void createAnalyzerStartParameters(Analyzer::AnalyzerStartParameters *pStartParameters, BlackBerryRunConfiguration* runConfiguration, ProjectExplorer::RunMode mode)
{
    QTC_ASSERT(pStartParameters, return);
    pStartParameters->runMode = mode;
    if (mode == ProjectExplorer::QmlProfilerRunMode)
        pStartParameters->startMode = Analyzer::StartLocal;

    ProjectExplorer::Target *target = runConfiguration->target();
    ProjectExplorer::Kit *kit = target->kit();

    ProjectExplorer::IDevice::ConstPtr device = ProjectExplorer::DeviceKitInformation::device(kit);
    if (device) {
        pStartParameters->connParams = device->sshParameters();
        pStartParameters->analyzerHost = device->qmlProfilerHost();
    }
    pStartParameters->sysroot = ProjectExplorer::SysRootKitInformation::sysRoot(kit).toString();

    Debugger::DebuggerRunConfigurationAspect *aspect = runConfiguration->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    if (aspect)
        pStartParameters->analyzerPort = aspect->qmlDebugServerPort();
}

ProjectExplorer::RunControl *BlackBerryRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration,
        ProjectExplorer::RunMode mode, QString *errorMessage)
{
    BlackBerryRunConfiguration *rc = qobject_cast<BlackBerryRunConfiguration *>(runConfiguration);
    if (!rc)
        return 0;

    BlackBerryDeployConfiguration *activeDeployConf = qobject_cast<BlackBerryDeployConfiguration *>(
                rc->target()->activeDeployConfiguration());
    if (!activeDeployConf) {
        if (errorMessage)
            *errorMessage = tr("No active deploy configuration");
        return 0;
    }

    if (mode == ProjectExplorer::NormalRunMode) {
        BlackBerryRunControl *runControl = new BlackBerryRunControl(rc);
        m_activeRunControls[rc->key()] = runControl;
        return runControl;
    }
    if (mode == ProjectExplorer::QmlProfilerRunMode) {
        QtSupport::BaseQtVersion *qtVer = QtSupport::QtKitInformation::qtVersion(rc->target()->kit());
        if (qtVer && qtVer->qtVersion() <= QtSupport::QtVersionNumber(4, 8, 6))
            Core::MessageManager::write(tr("Target Qt version (%1) might not support QML profiling. "
                "Cascades applications are not affected and should work as expected. "
                "For more info see http://qt-project.org/wiki/Qt-Creator-with-BlackBerry-10")
                .arg(qtVer->qtVersionString()), Core::MessageManager::Flash
            );

        Analyzer::AnalyzerStartParameters params;
        createAnalyzerStartParameters(&params, rc, mode);

        Analyzer::AnalyzerRunControl *runControl = Analyzer::AnalyzerManager::createRunControl(params, runConfiguration);
        BlackBerryApplicationRunner::LaunchFlags launchFlags(BlackBerryApplicationRunner::QmlDebugLaunch
            | BlackBerryApplicationRunner::QmlDebugLaunchBlocking
            | BlackBerryApplicationRunner::QmlProfilerLaunch);
        BlackBerryApplicationRunner *runner = new BlackBerryApplicationRunner(launchFlags, rc, runControl);

        connect(runner, SIGNAL(finished()), runControl, SLOT(notifyRemoteFinished()));
        connect(runner, SIGNAL(output(QString, Utils::OutputFormat)),
                runControl, SLOT(logApplicationMessage(QString, Utils::OutputFormat)));
        connect(runControl, SIGNAL(starting(const Analyzer::AnalyzerRunControl*)),
                runner, SLOT(start()));
        connect(runControl, SIGNAL(finished()), runner, SLOT(stop()));
        return runControl;
    }
    Debugger::DebuggerRunControl * const runControl =
            Debugger::DebuggerPlugin::createDebugger(startParameters(rc), runConfiguration, errorMessage);
    if (!runControl)
        return 0;

    new BlackBerryDebugSupport(rc, runControl);
    m_activeRunControls[rc->key()] = runControl;
    return runControl;
}

Debugger::DebuggerStartParameters BlackBerryRunControlFactory::startParameters(
        const BlackBerryRunConfiguration *runConfig)
{
    Debugger::DebuggerStartParameters params;
    ProjectExplorer::Target *target = runConfig->target();
    ProjectExplorer::Kit *k = target->kit();

    params.startMode = Debugger::AttachToRemoteServer;
    params.debuggerCommand = Debugger::DebuggerKitInformation::debuggerCommand(k).toString();
    params.sysRoot = ProjectExplorer::SysRootKitInformation::sysRoot(k).toString();
    params.useCtrlCStub = true;

    if (ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k))
        params.toolChainAbi = tc->targetAbi();

    params.executable = runConfig->localExecutableFilePath();
    params.displayName = runConfig->displayName();
    params.remoteSetupNeeded = true;

    Debugger::DebuggerRunConfigurationAspect *aspect
            = runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    if (aspect->useQmlDebugger()) {
        BlackBerryDeviceConfiguration::ConstPtr device = BlackBerryDeviceConfiguration::device(runConfig->target()->kit());
        if (device) {
            params.qmlServerAddress = device->sshParameters().host;
            params.qmlServerPort = aspect->qmlDebugServerPort();
            params.languages |= Debugger::QmlLanguage;
        }
    }
    if (aspect->useCppDebugger())
        params.languages |= Debugger::CppLanguage;

    if (const ProjectExplorer::Project *project = runConfig->target()->project()) {
        params.projectSourceDirectory = project->projectDirectory().toString();
        if (const ProjectExplorer::BuildConfiguration *buildConfig = runConfig->target()->activeBuildConfiguration())
            params.projectBuildDirectory = buildConfig->buildDirectory().toString();
        params.projectSourceFiles = project->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
    }

    BlackBerryQtVersion *qtVersion =
            dynamic_cast<BlackBerryQtVersion *>(QtSupport::QtKitInformation::qtVersion(k));
    if (qtVersion)
        params.solibSearchPath = QnxUtils::searchPaths(qtVersion);

    return params;
}
