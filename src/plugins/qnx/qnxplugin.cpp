/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxplugin.h"

#include "qnxanalyzesupport.h"
#include "qnxconfigurationmanager.h"
#include "qnxconstants.h"
#include "qnxdebugsupport.h"
#include "qnxdevice.h"
#include "qnxdevicefactory.h"
#include "qnxqtversion.h"
#include "qnxqtversionfactory.h"
#include "qnxrunconfiguration.h"
#include "qnxsettingspage.h"
#include "qnxtoolchain.h"
#include "qnxutils.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

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
#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>

#include <qtsupport/qtkitinformation.h>

#include <QAction>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

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
        setDefaultDisplayName(QCoreApplication::translate("Qnx::Internal::QnxDeployConfiguration",
                                                          "Deploy to QNX Device"));
        addSupportedTargetDeviceType(QnxDeviceFactory::deviceType());
        setUseDeploymentDataView();

        addInitialStep(DeviceCheckBuildStep::stepId());
        addInitialStep(RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep::stepId());
        addInitialStep(RemoteLinux::GenericDirectUploadStep::stepId());
    }
};

class QnxPluginPrivate
{
public:
    void updateDebuggerActions();

    QAction *m_debugSeparator = nullptr;
    QAction m_attachToQnxApplication{QnxPlugin::tr("Attach to remote QNX application..."), nullptr};

    QnxConfigurationManager configurationFactory;
    QnxQtVersionFactory qtVersionFactory;
    QnxDeviceFactory deviceFactory;
    QnxDeployConfigurationFactory deployConfigFactory;
    GenericQnxDeployStepFactory<RemoteLinux::GenericDirectUploadStep> directUploadDeployFactory;
    GenericQnxDeployStepFactory<RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep> checkForFreeDiskSpaceDeployFactory;
    GenericQnxDeployStepFactory<DeviceCheckBuildStep> checkBuildDeployFactory;
    QnxRunConfigurationFactory runConfigFactory;
    QnxSettingsPage settingsPage;
    QnxToolChainFactory toolChainFactory;
};

static QnxPluginPrivate *dd = nullptr;

QnxPlugin::~QnxPlugin()
{
    delete dd;
}

bool QnxPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    dd = new QnxPluginPrivate;

    auto constraint = [](RunConfiguration *runConfig) {
        if (!runConfig->isEnabled()
                || !runConfig->id().name().startsWith(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX)) {
            return false;
        }

        auto dev = DeviceKitInformation::device(runConfig->target()->kit())
                .dynamicCast<const QnxDevice>();
        return !dev.isNull();
    };

    RunControl::registerWorker<SimpleTargetRunner>
            (ProjectExplorer::Constants::NORMAL_RUN_MODE, constraint);
    RunControl::registerWorker<QnxDebugSupport>
            (ProjectExplorer::Constants::DEBUG_RUN_MODE, constraint);
    RunControl::registerWorker<QnxQmlProfilerSupport>
            (ProjectExplorer::Constants::QML_PROFILER_RUN_MODE, constraint);

    return true;
}

void QnxPlugin::extensionsInitialized()
{
    // Attach support
    connect(&dd->m_attachToQnxApplication, &QAction::triggered,
            this, [] { QnxAttachDebugSupport::showProcessesDialog(); });

    Core::ActionContainer *mstart = Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING);
    mstart->appendGroup(Constants::QNX_DEBUGGING_GROUP);
    mstart->addSeparator(Core::Context(Core::Constants::C_GLOBAL), Constants::QNX_DEBUGGING_GROUP,
                         &dd->m_debugSeparator);

    Core::Command *cmd = Core::ActionManager::registerAction
            (&dd->m_attachToQnxApplication, "Debugger.AttachToQnxApplication");
    mstart->addAction(cmd, Constants::QNX_DEBUGGING_GROUP);

    connect(KitManager::instance(), &KitManager::kitsChanged,
            this, [] { dd->updateDebuggerActions(); });
}

void QnxPluginPrivate::updateDebuggerActions()
{
    bool hasValidQnxKit = false;

    auto matcher = DeviceTypeKitInformation::deviceTypePredicate(Constants::QNX_QNX_OS_TYPE);
    foreach (Kit *qnxKit, KitManager::kits(matcher)) {
        if (qnxKit->isValid() && !DeviceKitInformation::device(qnxKit).isNull()) {
            hasValidQnxKit = true;
            break;
        }
    }

    m_attachToQnxApplication.setVisible(hasValidQnxKit);
    m_debugSeparator->setVisible(hasValidQnxKit);
}

} // Internal
} // Qnx
