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

class QnxUploadStep : public RemoteLinux::GenericDirectUploadStep
{
public:
    QnxUploadStep(BuildStepList *bsl, Utils::Id id) : GenericDirectUploadStep(bsl, id, false) {}
    static Utils::Id stepId() { return "Qnx.DirectUploadStep"; }
};

template <class Step>
class GenericQnxDeployStepFactory : public BuildStepFactory
{
public:
    GenericQnxDeployStepFactory()
    {
        registerStep<Step>(Step::stepId());
        setDisplayName(Step::displayName());
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
        addInitialStep(DeviceCheckBuildStep::stepId());
        addInitialStep(QnxUploadStep::stepId());
    }
};

class QnxPluginPrivate
{
public:
    void updateDebuggerActions();

    QAction *m_debugSeparator = nullptr;
    QAction m_attachToQnxApplication{Tr::tr("Attach to remote QNX application..."), nullptr};

    QnxConfigurationManager configurationFactory;
    QnxQtVersionFactory qtVersionFactory;
    QnxDeviceFactory deviceFactory;
    QnxDeployConfigurationFactory deployConfigFactory;
    GenericQnxDeployStepFactory<QnxUploadStep> directUploadDeployFactory;
    GenericQnxDeployStepFactory<RemoteLinux::MakeInstallStep> makeInstallDeployFactory;
    GenericQnxDeployStepFactory<DeviceCheckBuildStep> checkBuildDeployFactory;
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
