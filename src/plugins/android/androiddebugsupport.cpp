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

#include <projectexplorer/buildconfiguration.h>
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
                node->getProject()->activeBuildConfiguration());
    if (const Result<QByteArray> contents = jsonFile.fileContents()) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(*contents, &error);
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
            DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
            rp.setupPortsGatherer(runControl);
            rp.setSkipDebugServer(true);
            rp.setLldbPlatform("remote-android");

            auto androidRunner = new RunWorker(runControl, androidRecipe(runControl));

            BuildConfiguration *bc = runControl->buildConfiguration();
            Kit *kit = runControl->kit();
            rp.setStartMode(AttachToRemoteServer);
            const QString packageName = Internal::packageName(bc);
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

                FilePath buildDir = Internal::buildDirectory(bc);
                const RunConfiguration *activeRunConfig = bc->activeRunConfiguration();
                if (activeRunConfig)
                    solibSearchPath.append(activeRunConfig->buildTargetInfo().workingDirectory);
                solibSearchPath.append(buildDir);
                const FilePath androidLibsPath = androidBuildDirectory(bc)
                                                     .pathAppended("libs")
                                                     .pathAppended(apkDevicePreferredAbi(bc));
                solibSearchPath.append(androidLibsPath);
                FilePath::removeDuplicates(solibSearchPath);
                rp.setSolibSearchPath(solibSearchPath);
                qCDebug(androidDebugSupportLog).noquote() << "SoLibSearchPath: " << solibSearchPath;
                rp.setSymbolFile(androidAppProcessDir(bc).pathAppended("app_process"));
                rp.setUseExtendedRemote(true);
                const QString devicePreferredAbi = apkDevicePreferredAbi(bc);
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
            auto debugger = createDebuggerWorker(runControl, rp);
            debugger->addStartDependency(androidRunner);

            QObject::connect(debugger, &RunWorker::started, debugger, [runControl, packageName] {
                qCDebug(androidDebugSupportLog) << "Starting debugger - package name: " << packageName
                                                << ", PID: " << runControl->attachPid().pid();
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
