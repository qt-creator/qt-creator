// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbplugin.h"

#include "device-detection/devicedetector.h"
#include "qdbconstants.h"
#include "qdbdevice.h"
#include "qdbstopapplicationstep.h"
#include "qdbmakedefaultappstep.h"
#include "qdbdevicedebugsupport.h"
#include "qdbqtversion.h"
#include "qdbrunconfiguration.h"
#include "qdbutils.h"
#include "qdbtr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtversionfactory.h>

#include <remotelinux/remotelinux_constants.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>

#include <QAction>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb::Internal {

static FilePath flashWizardFilePath()
{
    return findTool(QdbTool::FlashingWizard);
}

static void startFlashingWizard()
{
    const FilePath filePath = flashWizardFilePath();
    if (HostOsInfo::isWindowsHost()) {
        if (Process::startDetached({"explorer.exe", {filePath.toUserOutput()}}))
            return;
    } else if (Process::startDetached({filePath, {}})) {
        return;
    }
    const QString message = Tr::tr("Flash wizard \"%1\" failed to start.");
    showMessage(message.arg(filePath.toUserOutput()), true);
}

static bool isFlashActionDisabled()
{
    QtcSettings * const settings = Core::ICore::settings();
    settings->beginGroup(settingsGroupKey());
    bool disabled = settings->value("flashActionDisabled", false).toBool();
    settings->endGroup();
    return disabled;
}

void registerFlashAction(QObject *parentForAction)
{
    if (isFlashActionDisabled())
        return;
    const FilePath fileName = flashWizardFilePath();
    if (!fileName.exists()) {
        const QString message = Tr::tr("Flash wizard executable \"%1\" not found.");
        showMessage(message.arg(fileName.toUserOutput()));
        return;
    }

    const char flashActionId[] = "Qdb.FlashAction";
    if (Core::ActionManager::command(flashActionId))
        return; // The action has already been registered.

    Core::ActionContainer *toolsContainer =
        Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_DEBUG, flashActionId);

    Core::Context globalContext(Core::Constants::C_GLOBAL);

    QAction *flashAction = new QAction(Tr::tr("Flash Boot to Qt Device"), parentForAction);
    Core::Command *flashCommand = Core::ActionManager::registerAction(flashAction,
                                                                      flashActionId,
                                                                      globalContext);
    QObject::connect(flashAction, &QAction::triggered, startFlashingWizard);
    toolsContainer->addAction(flashCommand, flashActionId);
}

class QdbDeployStepFactory : public BuildStepFactory
{
public:
    explicit QdbDeployStepFactory(Id existingStepId)
    {
        cloneStepCreator(existingStepId);
        setSupportedConfiguration(Constants::QdbDeployConfigurationId);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    }
};

class QdbDeployConfigurationFactory final : public DeployConfigurationFactory
{
public:
    QdbDeployConfigurationFactory()
    {
        setConfigBaseId(Constants::QdbDeployConfigurationId);
        addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
        setDefaultDisplayName(Tr::tr("Deploy to Boot2Qt target"));
        setUseDeploymentDataView();

        addInitialStep(RemoteLinux::Constants::MakeInstallStepId, [](Target *target) {
            const Project * const prj = target->project();
            return prj->deploymentKnowledge() == DeploymentKnowledge::Bad
                   && prj->hasMakeInstallEquivalent();
        });
        addInitialStep(Qdb::Constants::QdbStopApplicationStepId);
#ifdef Q_OS_WIN
        addInitialStep(RemoteLinux::Constants::DirectUploadStepId);
#else
        addInitialStep(RemoteLinux::Constants::GenericDeployStepId);
#endif
    }
};

class QdbPluginPrivate : public QObject
{
public:
    void setupDeviceDetection();

    QdbLinuxDeviceFactory m_qdbDeviceFactory;
    QdbQtVersionFactory m_qtVersionFactory;
    QdbDeployConfigurationFactory m_deployConfigFactory;
    QdbRunConfigurationFactory m_runConfigFactory;
    QdbStopApplicationStepFactory m_stopApplicationStepFactory;
    QdbMakeDefaultAppStepFactory m_makeDefaultAppStepFactory;

    QdbDeployStepFactory m_directUploadStepFactory{RemoteLinux::Constants::DirectUploadStepId};
    QdbDeployStepFactory m_rsyncDeployStepFactory{RemoteLinux::Constants::GenericDeployStepId};
    QdbDeployStepFactory m_makeInstallStepFactory{RemoteLinux::Constants::MakeInstallStepId};

    const QList<Id> supportedRunConfigs {
        m_runConfigFactory.runConfigurationId(),
        "QmlProjectManager.QmlRunConfiguration"
    };

    QdbRunWorkerFactory runWorkerFactory{supportedRunConfigs};
    QdbDebugWorkerFactory debugWorkerFactory{supportedRunConfigs};
    QdbQmlToolingWorkerFactory qmlToolingWorkerFactory{supportedRunConfigs};
    QdbPerfProfilerWorkerFactory perfRecorderWorkerFactory;

    DeviceDetector m_deviceDetector;
};

QdbPlugin::~QdbPlugin()
{
    delete d;
}

void QdbPlugin::initialize()
{
    d = new QdbPluginPrivate;

    registerFlashAction(this);
}

void QdbPlugin::extensionsInitialized()
{
    DeviceManager * const dm = DeviceManager::instance();
    if (dm->isLoaded()) {
        d->setupDeviceDetection();
    } else {
        connect(dm, &DeviceManager::devicesLoaded,
                d, &QdbPluginPrivate::setupDeviceDetection);
    }
}

ExtensionSystem::IPlugin::ShutdownFlag QdbPlugin::aboutToShutdown()
{
    d->m_deviceDetector.stop();

    return SynchronousShutdown;
}

void QdbPluginPrivate::setupDeviceDetection()
{
    m_deviceDetector.start();
}

} // Qdb::Internal
