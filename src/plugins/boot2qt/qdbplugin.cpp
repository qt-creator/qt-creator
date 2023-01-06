// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtversionfactory.h>

#include <remotelinux/genericdirectuploadstep.h>
#include <remotelinux/makeinstallstep.h>
#include <remotelinux/rsyncdeploystep.h>
#include <remotelinux/remotelinux_constants.h>

#include <utils/hostosinfo.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>

#include <QAction>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb {
namespace Internal {

static FilePath flashWizardFilePath()
{
    return findTool(QdbTool::FlashingWizard);
}

static void startFlashingWizard()
{
    const FilePath filePath = flashWizardFilePath();
    if (HostOsInfo::isWindowsHost()) {
        if (QtcProcess::startDetached({"explorer.exe", {filePath.toUserOutput()}}))
            return;
    } else if (QtcProcess::startDetached({filePath, {}})) {
        return;
    }
    const QString message =
            QCoreApplication::translate("Qdb", "Flash wizard \"%1\" failed to start.");
    showMessage(message.arg(filePath.toUserOutput()), true);
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
    const FilePath fileName = flashWizardFilePath();
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
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_DEBUG, flashActionId);

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

template <class Step>
class QdbDeployStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    explicit QdbDeployStepFactory(Id id)
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

    QdbDeployStepFactory<RemoteLinux::GenericDirectUploadStep>
        m_directUploadStepFactory{RemoteLinux::Constants::DirectUploadStepId};
    QdbDeployStepFactory<RemoteLinux::RsyncDeployStep>
        m_rsyncDeployStepFactory{RemoteLinux::Constants::RsyncDeployStepId};
    QdbDeployStepFactory<RemoteLinux::MakeInstallStep>
        m_makeInstallStepFactory{RemoteLinux::Constants::MakeInstallStepId};

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
