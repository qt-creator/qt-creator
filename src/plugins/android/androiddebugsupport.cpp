// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androiddebugsupport.h"

#include "androidconstants.h"
#include "androidrunner.h"
#include "androidmanager.h"
#include "androidqtversion.h"

#include <debugger/debuggerkitaspect.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

namespace {
static Q_LOGGING_CATEGORY(androidDebugSupportLog, "qtc.android.run.androiddebugsupport", QtWarningMsg)
}

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Android::Internal {

static FilePaths getSoLibSearchPath(const ProjectNode *node)
{
    if (!node)
        return {};

    FilePaths res;
    node->forEachProjectNode([&res](const ProjectNode *node) {
        const QStringList paths = node->data(Constants::AndroidSoLibPath).toStringList();
        res.append(Utils::transform(paths, &FilePath::fromUserInput));
    });

    const FilePath jsonFile = AndroidQtVersion::androidDeploymentSettings(
                node->getProject()->activeTarget());
    FileReader reader;
    if (reader.fetch(jsonFile)) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(reader.data(), &error);
        if (error.error == QJsonParseError::NoError) {
            auto rootObj = doc.object();
            auto it = rootObj.find("stdcpp-path");
            if (it != rootObj.constEnd())
                res.append(FilePath::fromUserInput(it.value().toString()));
        }
    }

    FilePath::removeDuplicates(res);
    return res;
}

static FilePaths getExtraLibs(const ProjectNode *node)
{
    if (!node)
        return {};

    const QStringList paths = node->data(Constants::AndroidExtraLibs).toStringList();
    FilePaths res = Utils::transform(paths, &FilePath::fromUserInput);

    FilePath::removeDuplicates(res);
    return res;
}

class AndroidDebugSupport : public Debugger::DebuggerRunTool
{
public:
    explicit AndroidDebugSupport(RunControl *runControl) : Debugger::DebuggerRunTool(runControl)
    {
        setId("AndroidDebugger");
        setLldbPlatform("remote-android");
        m_runner = new AndroidRunner(runControl, {});
        addStartDependency(m_runner);
    }

    void start() override;
    void stop() override;

private:
    AndroidRunner *m_runner = nullptr;
};

void AndroidDebugSupport::start()
{
    Target *target = runControl()->target();
    Kit *kit = target->kit();

    setStartMode(AttachToRemoteServer);
    const QString packageName = AndroidManager::packageName(target);
    setRunControlName(packageName);
    setUseContinueInsteadOfRun(true);
    setAttachPid(m_runner->pid());

    QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(kit);
    if (!HostOsInfo::isWindowsHost()
        && (qtVersion
            && AndroidConfigurations::currentConfig().ndkVersion(qtVersion)
                   >= QVersionNumber(11, 0, 0))) {
        qCDebug(androidDebugSupportLog) << "UseTargetAsync: " << true;
        setUseTargetAsync(true);
    }

    if (isCppDebugging()) {
        qCDebug(androidDebugSupportLog) << "C++ debugging enabled";
        const ProjectNode *node = target->project()->findNodeForBuildKey(runControl()->buildKey());
        FilePaths solibSearchPath = getSoLibSearchPath(node);
        if (qtVersion)
            solibSearchPath.append(qtVersion->qtSoPaths());
        const FilePaths extraLibs = getExtraLibs(node);
        solibSearchPath.append(extraLibs);

        FilePath buildDir = AndroidManager::buildDirectory(target);
        const RunConfiguration *activeRunConfig = target->activeRunConfiguration();
        if (activeRunConfig)
            solibSearchPath.append(activeRunConfig->buildTargetInfo().workingDirectory);
        solibSearchPath.append(buildDir);
        const FilePath androidLibsPath = AndroidManager::androidBuildDirectory(target)
                                         .pathAppended("libs")
                                         .pathAppended(AndroidManager::apkDevicePreferredAbi(target));
        solibSearchPath.append(androidLibsPath);
        FilePath::removeDuplicates(solibSearchPath);
        setSolibSearchPath(solibSearchPath);
        qCDebug(androidDebugSupportLog).noquote() << "SoLibSearchPath: " << solibSearchPath;
        setSymbolFile(AndroidManager::androidAppProcessDir(target).pathAppended("app_process"));
        setSkipExecutableValidation(true);
        setUseExtendedRemote(true);
        QString devicePreferredAbi = AndroidManager::apkDevicePreferredAbi(target);
        setAbi(AndroidManager::androidAbi2Abi(devicePreferredAbi));

        if (cppEngineType() == LldbEngineType) {
            QString deviceSerialNumber = AndroidManager::deviceSerialNumber(target);
            const int colonPos = deviceSerialNumber.indexOf(QLatin1Char(':'));
            if (colonPos > 0) {
                // When wireless debugging is used then the device serial number will include a port number
                // The port number must be removed to form a valid hostname
                deviceSerialNumber.truncate(colonPos);
            }
            setRemoteChannel("adb://" + deviceSerialNumber,
                             m_runner->debugServerPort().number());
        } else {
            QUrl debugServer;
            debugServer.setPort(m_runner->debugServerPort().number());
            debugServer.setHost(QHostAddress(QHostAddress::LocalHost).toString());
            setRemoteChannel(debugServer);
        }

        auto qt = static_cast<AndroidQtVersion *>(qtVersion);
        const int minimumNdk = qt ? qt->minimumNDK() : 0;

        int sdkVersion = qMax(AndroidManager::minimumSDK(kit), minimumNdk);
        if (qtVersion) {
            const FilePath ndkLocation =
                    AndroidConfigurations::currentConfig().ndkLocation(qtVersion);
            FilePath sysRoot = ndkLocation
                    / "platforms"
                    / QString("android-%1").arg(sdkVersion)
                    / devicePreferredAbi; // Legacy Ndk structure
            if (!sysRoot.exists())
                sysRoot = AndroidConfig::toolchainPathFromNdk(ndkLocation) / "sysroot";
            setSysRoot(sysRoot);
            qCDebug(androidDebugSupportLog).noquote() << "Sysroot: " << sysRoot.toUserOutput();
        }
    }
    if (isQmlDebugging()) {
        qCDebug(androidDebugSupportLog) << "QML debugging enabled. QML server: "
                                        << m_runner->qmlServer().toDisplayString();
        setQmlServer(m_runner->qmlServer());
        //TODO: Not sure if these are the right paths.
        if (qtVersion)
            addSearchDirectory(qtVersion->qmlPath());
    }

    qCDebug(androidDebugSupportLog) << "Starting debugger - package name: " << packageName
                                    << ", PID: " << m_runner->pid().pid();
    DebuggerRunTool::start();
}

void AndroidDebugSupport::stop()
{
    qCDebug(androidDebugSupportLog) << "Stop";
    DebuggerRunTool::stop();
}

// AndroidDebugWorkerFactory

AndroidDebugWorkerFactory::AndroidDebugWorkerFactory()
{
    setProduct<AndroidDebugSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    addSupportedRunConfig(Constants::ANDROID_RUNCONFIG_ID);
}

} // Android::Internal
