// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androiddebugsupport.h"

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidrunner.h"
#include "androidqtversion.h"
#include "androidutils.h"

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
#include <utils/qtcprocess.h>

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

// AndroidDebugWorkerFactory

class AndroidDebugWorkerFactory final : public RunWorkerFactory
{
public:
    AndroidDebugWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            DebuggerRunTool *debugger = new DebuggerRunTool(runControl);
            debugger->setId("AndroidDebugger");
            DebuggerRunParameters &rp = debugger->runParameters();
            rp.setLldbPlatform("remote-android");

            auto runner = new AndroidRunner(runControl);
            debugger->addStartDependency(runner);

            Target *target = runControl->target();
            Kit *kit = runControl->kit();
            rp.setStartMode(AttachToRemoteServer);
            const QString packageName = Internal::packageName(target);
            rp.setDisplayName(packageName);
            rp.setUseContinueInsteadOfRun(true);

            QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(kit);
            if (!HostOsInfo::isWindowsHost()
                && (qtVersion && AndroidConfig::ndkVersion(qtVersion) >= QVersionNumber(11, 0, 0))) {
                qCDebug(androidDebugSupportLog) << "UseTargetAsync: " << true;
                rp.setUseTargetAsync(true);
            }

            if (rp.isCppDebugging()) {
                qCDebug(androidDebugSupportLog) << "C++ debugging enabled";
                const ProjectNode *node = runControl->project()->findNodeForBuildKey(runControl->buildKey());
                FilePaths solibSearchPath = getSoLibSearchPath(node);
                if (qtVersion)
                    solibSearchPath.append(qtVersion->qtSoPaths());
                const FilePaths extraLibs = getExtraLibs(node);
                solibSearchPath.append(extraLibs);

                FilePath buildDir = Internal::buildDirectory(target);
                const RunConfiguration *activeRunConfig = target->activeRunConfiguration();
                if (activeRunConfig)
                    solibSearchPath.append(activeRunConfig->buildTargetInfo().workingDirectory);
                solibSearchPath.append(buildDir);
                const FilePath androidLibsPath = androidBuildDirectory(target)
                                                     .pathAppended("libs")
                                                     .pathAppended(apkDevicePreferredAbi(target));
                solibSearchPath.append(androidLibsPath);
                FilePath::removeDuplicates(solibSearchPath);
                rp.setSolibSearchPath(solibSearchPath);
                qCDebug(androidDebugSupportLog).noquote() << "SoLibSearchPath: " << solibSearchPath;
                rp.setSymbolFile(androidAppProcessDir(target).pathAppended("app_process"));
                rp.setUseExtendedRemote(true);
                const QString devicePreferredAbi = apkDevicePreferredAbi(target);
                rp.setToolChainAbi(androidAbi2Abi(devicePreferredAbi));

                auto qt = static_cast<AndroidQtVersion *>(qtVersion);
                const int minimumNdk = qt ? qt->minimumNDK() : 0;

                int sdkVersion = qMax(Internal::minimumSDK(kit), minimumNdk);
                if (qtVersion) {
                    const FilePath ndkLocation = AndroidConfig::ndkLocation(qtVersion);
                    FilePath sysRoot = ndkLocation
                                       / "platforms"
                                       / QString("android-%1").arg(sdkVersion)
                                       / devicePreferredAbi; // Legacy Ndk structure
                    if (!sysRoot.exists())
                        sysRoot = AndroidConfig::toolchainPathFromNdk(ndkLocation) / "sysroot";
                    rp.setSysRoot(sysRoot);
                    qCDebug(androidDebugSupportLog).noquote() << "Sysroot: " << sysRoot.toUserOutput();
                }
            }
            if (rp.isQmlDebugging()) {
                qCDebug(androidDebugSupportLog) << "QML debugging enabled. QML server: "
                                                << runControl->qmlChannel().toDisplayString();
                //TODO: Not sure if these are the right paths.
                if (qtVersion)
                    rp.addSearchDirectory(qtVersion->qmlPath());
            }
            QObject::connect(debugger, &RunWorker::started, runner, [runner, packageName] {
                qCDebug(androidDebugSupportLog) << "Starting debugger - package name: " << packageName
                                                << ", PID: " << runner->pid().pid();
            });

            QObject::connect(runner, &RunWorker::started, debugger, [runControl, debugger, runner] {
                DebuggerRunParameters &rp = debugger->runParameters();
                rp.setAttachPid(runner->pid());
                if (rp.isCppDebugging()) {
                    if (rp.cppEngineType() == LldbEngineType) {
                        QString deviceSerialNumber = Internal::deviceSerialNumber(runControl->target());
                        const int colonPos = deviceSerialNumber.indexOf(QLatin1Char(':'));
                        if (colonPos > 0) {
                            // When wireless debugging is used then the device serial number will include a port number
                            // The port number must be removed to form a valid hostname
                            deviceSerialNumber.truncate(colonPos);
                        }
                        rp.setRemoteChannel("adb://" + deviceSerialNumber, runner->debugServerPort().number());
                    } else {
                        QUrl debugServer;
                        debugServer.setPort(runner->debugServerPort().number());
                        debugServer.setHost(QHostAddress(QHostAddress::LocalHost).toString());
                        rp.setRemoteChannel(debugServer);
                    }
                }
                if (rp.isQmlDebugging())
                    rp.setQmlServer(runControl->qmlChannel());
            });

            return debugger;
        });
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(Constants::ANDROID_RUNCONFIG_ID);
    }
};

void setupAndroidDebugWorker()
{
    static AndroidDebugWorkerFactory theAndroidDebugWorkerFactory;
}

} // Android::Internal
