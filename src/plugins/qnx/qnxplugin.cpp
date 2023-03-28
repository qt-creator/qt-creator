// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxanalyzesupport.h"
#include "qnxconfigurationmanager.h"
#include "qnxconstants.h"
#include "qnxdebugsupport.h"
#include "qnxdevice.h"
#include "qnxqtversion.h"
#include "qnxrunconfiguration.h"
#include "qnxsettingspage.h"
#include "qnxtoolchain.h"
#include "qnxtr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/devicesupport/devicecheckbuildstep.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <remotelinux/genericdirectuploadstep.h>
#include <remotelinux/makeinstallstep.h>
#include <remotelinux/remotelinux_constants.h>

#include <QAction>

using namespace ProjectExplorer;

namespace Qnx::Internal {

// FIXME: Remove...
class QnxUploadStepFactory : public RemoteLinux::GenericDirectUploadStepFactory
{
public:
    QnxUploadStepFactory()
    {
        registerStep<RemoteLinux::GenericDirectUploadStep>(Constants::QNX_DIRECT_UPLOAD_STEP_ID);
    }
};

template <class Factory>
class QnxDeployStepFactory : public RemoteLinux::MakeInstallStepFactory
{
public:
    QnxDeployStepFactory()
    {
        setSupportedConfiguration(Constants::QNX_QNX_DEPLOYCONFIGURATION_ID);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    }
};

class QnxDeployConfigurationFactory : public DeployConfigurationFactory
{
public:
    QnxDeployConfigurationFactory()
    {
        setConfigBaseId(Constants::QNX_QNX_DEPLOYCONFIGURATION_ID);
        setDefaultDisplayName(Tr::tr("Deploy to QNX Device"));
        addSupportedTargetDeviceType(Constants::QNX_QNX_OS_TYPE);
        setUseDeploymentDataView();

        addInitialStep(RemoteLinux::Constants::MakeInstallStepId, [](Target *target) {
            const Project * const prj = target->project();
            return prj->deploymentKnowledge() == DeploymentKnowledge::Bad
                    && prj->hasMakeInstallEquivalent();
        });
        addInitialStep(ProjectExplorer::Constants::DEVICE_CHECK_STEP);
        addInitialStep(Constants::QNX_DIRECT_UPLOAD_STEP_ID);
    }
};

class QnxPluginPrivate
{
public:
    void updateDebuggerActions();

    QAction *m_debugSeparator = nullptr;
    QAction m_attachToQnxApplication{Tr::tr("Attach to remote QNX application..."), nullptr};

    QnxConfigurationManager configurationManager;
    QnxQtVersionFactory qtVersionFactory;
    QnxDeviceFactory deviceFactory;
    QnxDeployConfigurationFactory deployConfigFactory;
    QnxDeployStepFactory<QnxUploadStepFactory> directUploadDeployFactory;
    QnxDeployStepFactory<RemoteLinux::MakeInstallStepFactory> makeInstallDeployFactory;
    QnxDeployStepFactory<DeviceCheckBuildStepFactory> checkBuildDeployFactory;
    QnxRunConfigurationFactory runConfigFactory;
    QnxSettingsPage settingsPage;
    QnxToolChainFactory toolChainFactory;
    SimpleTargetRunnerFactory runWorkerFactory{{runConfigFactory.runConfigurationId()}};
    QnxDebugWorkerFactory debugWorkerFactory;
    QnxQmlProfilerWorkerFactory qmlProfilerWorkerFactory;
};

class QnxPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Qnx.json")

public:
    ~QnxPlugin() final { delete d; }

private:
    void initialize() final { d = new QnxPluginPrivate; }
    void extensionsInitialized() final;

    QnxPluginPrivate *d = nullptr;
};

void QnxPlugin::extensionsInitialized()
{
    // Can't do yet as not all devices are around.
    connect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
            &d->configurationManager, &QnxConfigurationManager::restoreConfigurations);

    // Attach support
    connect(&d->m_attachToQnxApplication, &QAction::triggered, this, &showAttachToProcessDialog);

    const char QNX_DEBUGGING_GROUP[] = "Debugger.Group.Qnx";

    Core::ActionContainer *mstart = Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING);
    mstart->appendGroup(QNX_DEBUGGING_GROUP);
    mstart->addSeparator(Core::Context(Core::Constants::C_GLOBAL), QNX_DEBUGGING_GROUP,
                         &d->m_debugSeparator);

    Core::Command *cmd = Core::ActionManager::registerAction
            (&d->m_attachToQnxApplication, "Debugger.AttachToQnxApplication");
    mstart->addAction(cmd, QNX_DEBUGGING_GROUP);

    connect(KitManager::instance(), &KitManager::kitsChanged,
            this, [this] { d->updateDebuggerActions(); });
}

void QnxPluginPrivate::updateDebuggerActions()
{
    auto isQnxKit = [](const Kit *kit) {
        return DeviceTypeKitAspect::deviceTypeId(kit) == Constants::QNX_QNX_OS_TYPE
               && !DeviceKitAspect::device(kit).isNull() && kit->isValid();
    };

    const bool hasValidQnxKit = KitManager::kit(isQnxKit) != nullptr;

    m_attachToQnxApplication.setVisible(hasValidQnxKit);
    m_debugSeparator->setVisible(hasValidQnxKit);
}

} // Qnx::Internal

#include "qnxplugin.moc"
