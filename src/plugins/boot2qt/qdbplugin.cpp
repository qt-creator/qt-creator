/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qdbplugin.h"

#include "device-detection/devicedetector.h"
#include "qdbdeployconfigurationfactory.h"
#include "qdbstopapplicationstep.h"
#include "qdbmakedefaultappstep.h"
#include "qdbdevicedebugsupport.h"
#include "qdbqtversion.h"
#include "qdbrunconfiguration.h"
#include "qdbutils.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtversionfactory.h>

#include <remotelinux/genericdirectuploadstep.h>
#include <remotelinux/makeinstallstep.h>
#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>
#include <remotelinux/remotelinux_constants.h>

#include <utils/hostosinfo.h>
#include <utils/fileutils.h>

#include <QAction>
#include <QFileInfo>
#include <QProcess>

using namespace ProjectExplorer;

namespace Qdb {
namespace Internal {

static Utils::FilePath flashWizardFilePath()
{
    return findTool(QdbTool::FlashingWizard);
}

static void startFlashingWizard()
{
    const QString filePath = flashWizardFilePath().toUserOutput();
    if (Utils::HostOsInfo::isWindowsHost()) {
        if (QProcess::startDetached(QLatin1String("explorer.exe"), {filePath}))
            return;
    } else if (QProcess::startDetached(filePath, {})) {
        return;
    }
    const QString message =
            QCoreApplication::translate("Qdb", "Flash wizard \"%1\" failed to start.");
    showMessage(message.arg(filePath), true);
}

static bool isFlashActionDisabled()
{
    QSettings * const settings = Core::ICore::settings();
    settings->beginGroup(settingsGroupKey());
    bool disabled = settings->value("flashActionDisabled", false).toBool();
    settings->endGroup();
    return disabled;
}

void registerFlashAction(QObject *parentForAction)
{
    if (isFlashActionDisabled())
        return;
    const Utils::FilePath fileName = flashWizardFilePath();
    if (!fileName.exists()) {
        const QString message =
                QCoreApplication::translate("Qdb", "Flash wizard executable \"%1\" not found.");
        showMessage(message.arg(fileName.toString()));
        return;
    }

    const char flashActionId[] = "Qdb.FlashAction";
    if (Core::ActionManager::command(flashActionId))
        return; // The action has already been registered.

    Core::ActionContainer *toolsContainer =
        Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_OPTIONS,
                                flashActionId);

    Core::Context globalContext(Core::Constants::C_GLOBAL);

    const QString actionText = QCoreApplication::translate("Qdb", "Flash Boot to Qt Device");
    QAction *flashAction = new QAction(actionText, parentForAction);
    Core::Command *flashCommand = Core::ActionManager::registerAction(flashAction,
                                                                      flashActionId,
                                                                      globalContext);
    QObject::connect(flashAction, &QAction::triggered, startFlashingWizard);
    toolsContainer->addAction(flashCommand, flashActionId);
}

class QdbQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    QdbQtVersionFactory()
    {
        setQtVersionCreator([] { return new QdbQtVersion; });
        setSupportedType("Qdb.EmbeddedLinuxQt");
        setPriority(99);
        setRestrictionChecker([](const SetupData &setup) {
            return setup.platforms.contains("boot2qt");
        });
    }
};

class QdbDeviceRunSupport : public SimpleTargetRunner
{
public:
    QdbDeviceRunSupport(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        setStarter([this, runControl] {
            Runnable r = runControl->runnable();
            r.commandLineArguments = r.executable.toString() + ' ' + r.commandLineArguments;
            r.executable = Utils::FilePath::fromString(Constants::AppcontrollerFilepath);
            doStart(r, runControl->device());
        });
    }
};

template <class Step>
class QdbDeployStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    explicit QdbDeployStepFactory(Utils::Id id)
    {
        registerStep<Step>(id);
        setDisplayName(Step::displayName());
        setSupportedConfiguration(Constants::QdbDeployConfigurationId);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
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

    QdbDeployStepFactory<RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep>
        m_checkForFreeDiskSpaceStepFactory{RemoteLinux::Constants::CheckForFreeDiskSpaceId};
    QdbDeployStepFactory<RemoteLinux::GenericDirectUploadStep>
        m_directUploadStepFactory{RemoteLinux::Constants::DirectUploadStepId};
    QdbDeployStepFactory<RemoteLinux::MakeInstallStep>
        m_makeInstallStepFactory{RemoteLinux::Constants::MakeInstallStepId};

    const QList<Utils::Id> supportedRunConfigs {
        m_runConfigFactory.runConfigurationId(),
        "QmlProjectManager.QmlRunConfiguration"
    };

    RunWorkerFactory runWorkerFactory{
        RunWorkerFactory::make<QdbDeviceRunSupport>(),
        {ProjectExplorer::Constants::NORMAL_RUN_MODE},
        supportedRunConfigs,
        {Qdb::Constants::QdbLinuxOsType}
    };
    RunWorkerFactory debugWorkerFactory{
        RunWorkerFactory::make<QdbDeviceDebugSupport>(),
        {ProjectExplorer::Constants::DEBUG_RUN_MODE},
        supportedRunConfigs,
        {Qdb::Constants::QdbLinuxOsType}
    };
    RunWorkerFactory qmlToolWorkerFactory{
        RunWorkerFactory::make<QdbDeviceQmlToolingSupport>(),
        {ProjectExplorer::Constants::QML_PROFILER_RUN_MODE,
         ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE},
        supportedRunConfigs,
        {Qdb::Constants::QdbLinuxOsType}
    };
    RunWorkerFactory perfRecorderFactory{
        RunWorkerFactory::make<QdbDevicePerfProfilerSupport>(),
        {"PerfRecorder"},
        {},
        {Qdb::Constants::QdbLinuxOsType}
    };

    DeviceDetector m_deviceDetector;
};

QdbPlugin::~QdbPlugin()
{
    delete d;
}

bool QdbPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new QdbPluginPrivate;

    registerFlashAction(this);

    return true;
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

} // Internal
} // Qdb
