// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <extensionsystem/iplugin.h>

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtversionfactory.h>

#include <remotelinux/remotelinux_constants.h>

#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

using namespace Core;
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
    } else if (Process::startDetached(CommandLine{filePath})) {
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
    if (ActionManager::command(flashActionId))
        return; // The action has already been registered.

    ActionContainer *toolsContainer = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_DEBUG, flashActionId);

    ActionBuilder flashAction(parentForAction, flashActionId);
    flashAction.setText(Tr::tr("Flash Boot to Qt Device"));
    flashAction.addToContainer(Core::Constants::M_TOOLS, flashActionId);
    flashAction.addOnTriggered(&startFlashingWizard);
}

class QdbDeployStepFactory : public BuildStepFactory
{
public:
    explicit QdbDeployStepFactory(Id existingStepId)
    {
        cloneStepCreator(existingStepId);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedDeviceType(Constants::QdbLinuxOsType);
    }
};

class QdbDeployConfigurationFactory final : public DeployConfigurationFactory
{
public:
    QdbDeployConfigurationFactory()
    {
        setConfigBaseId(Constants::QdbDeployConfigurationId);
        addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
        setDefaultDisplayName(Tr::tr("Deploy to Boot to Qt target"));
        setUseDeploymentDataView();

        addInitialStep(RemoteLinux::Constants::MakeInstallStepId, [](BuildConfiguration *bc) {
            const Project * const prj = bc->project();
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

class QdbPluginPrivate final : public QObject
{
public:
    void setupDeviceDetection() { m_deviceDetector.start(); }

    QdbDeployConfigurationFactory m_deployConfigFactory;
    QdbRunConfigurationFactory m_runConfigFactory;
    QdbStopApplicationStepFactory m_stopApplicationStepFactory;
    QdbMakeDefaultAppStepFactory m_makeDefaultAppStepFactory;

    QdbDeployStepFactory m_directUploadStepFactory{RemoteLinux::Constants::DirectUploadStepId};
    QdbDeployStepFactory m_rsyncDeployStepFactory{RemoteLinux::Constants::GenericDeployStepId};
    QdbDeployStepFactory m_makeInstallStepFactory{RemoteLinux::Constants::MakeInstallStepId};

    DeviceDetector m_deviceDetector;
};

class QdbPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Boot2Qt.json")

public:
    ~QdbPlugin() final { delete d; }

private:
    void initialize() final
    {
        setupQdbLinuxDevice();
        setupQdbQtVersion();
        setupQdbRunWorkers();

        d = new QdbPluginPrivate;

        registerFlashAction(this);
    }

    void extensionsInitialized() final
    {
        if (DeviceManager::isLoaded()) {
            d->setupDeviceDetection();
        } else {
            connect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
                    d, &QdbPluginPrivate::setupDeviceDetection);
        }
    }

    ShutdownFlag aboutToShutdown() final
    {
        d->m_deviceDetector.stop();

        return SynchronousShutdown;
    }

    class QdbPluginPrivate *d = nullptr;
};

} // Qdb::Internal

#include "qdbplugin.moc"
