/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androiddebugsupport.h"

#include "androidconstants.h"
#include "androidglobal.h"
#include "androidrunner.h"
#include "androidmanager.h"
#include "androidqtversion.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/hostosinfo.h>

#include <QDirIterator>
#include <QHostAddress>
#include <QJsonDocument>
#include <QLoggingCategory>

namespace {
static Q_LOGGING_CATEGORY(androidDebugSupportLog, "qtc.android.run.androiddebugsupport", QtWarningMsg)
}

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

static QStringList uniquePaths(const QStringList &files)
{
    QSet<QString> paths;
    for (const QString &file : files)
        paths << QFileInfo(file).absolutePath();
    return Utils::toList(paths);
}

static QStringList getSoLibSearchPath(const ProjectNode *node)
{
    if (!node)
        return {};

    QStringList res;
    node->forEachProjectNode([&res](const ProjectNode *node) {
         res.append(node->data(Constants::AndroidSoLibPath).toStringList());
    });

    const QString jsonFile = node->data(Android::Constants::AndroidDeploySettingsFile).toString();
    QFile deploymentSettings(jsonFile);
    if (deploymentSettings.open(QIODevice::ReadOnly)) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(deploymentSettings.readAll(), &error);
        if (error.error == QJsonParseError::NoError) {
            auto rootObj = doc.object();
            auto it = rootObj.find("stdcpp-path");
            if (it != rootObj.constEnd())
                res.append(QFileInfo(it.value().toString()).absolutePath());
        }
    }

    res.removeDuplicates();
    return res;
}

static QStringList getExtraLibs(const ProjectNode *node)
{
    if (!node)
        return {};
    return node->data(Android::Constants::AndroidExtraLibs).toStringList();
}

AndroidDebugSupport::AndroidDebugSupport(RunControl *runControl, const QString &intentName)
    : Debugger::DebuggerRunTool(runControl)
{
    setId("AndroidDebugger");
    setLldbPlatform("remote-android");
    m_runner = new AndroidRunner(runControl, intentName);
    addStartDependency(m_runner);
}

void AndroidDebugSupport::start()
{
    Target *target = runControl()->target();
    Kit *kit = target->kit();

    setStartMode(AttachToRemoteServer);
    const QString packageName = AndroidManager::packageName(target);
    setRunControlName(packageName);
    setUseContinueInsteadOfRun(true);
    setAttachPid(m_runner->pid());

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(kit);
    if (!Utils::HostOsInfo::isWindowsHost()
        && (qtVersion
            && AndroidConfigurations::currentConfig().ndkVersion(qtVersion)
                   >= QVersionNumber(11, 0, 0))) {
        qCDebug(androidDebugSupportLog) << "UseTargetAsync: " << true;
        setUseTargetAsync(true);
    }

    if (isCppDebugging()) {
        qCDebug(androidDebugSupportLog) << "C++ debugging enabled";
        const ProjectNode *node = target->project()->findNodeForBuildKey(runControl()->buildKey());
        QStringList solibSearchPath = getSoLibSearchPath(node);
        QStringList extraLibs = getExtraLibs(node);
        if (qtVersion)
            solibSearchPath.append(qtVersion->qtSoPaths());
        solibSearchPath.append(uniquePaths(extraLibs));
        solibSearchPath.append(runControl()->buildDirectory().toString());
        solibSearchPath.removeDuplicates();
        setSolibSearchPath(solibSearchPath);
        qCDebug(androidDebugSupportLog) << "SoLibSearchPath: "<<solibSearchPath;
        setSymbolFile(runControl()->buildDirectory().pathAppended("app_process"));
        setSkipExecutableValidation(true);
        setUseExtendedRemote(true);
        QString devicePreferredAbi = AndroidManager::apkDevicePreferredAbi(target);
        setAbi(AndroidManager::androidAbi2Abi(devicePreferredAbi));

        QUrl debugServer;
        debugServer.setPort(m_runner->debugServerPort().number());
        if (cppEngineType() == LldbEngineType) {
            debugServer.setScheme("adb");
            debugServer.setHost(AndroidManager::deviceSerialNumber(target));
            setRemoteChannel(debugServer.toString());
        } else {
            debugServer.setHost(QHostAddress(QHostAddress::LocalHost).toString());
            setRemoteChannel(debugServer);
        }

        auto qt = static_cast<AndroidQtVersion *>(qtVersion);
        const int minimumNdk = qt ? qt->minimumNDK() : 0;

        int sdkVersion = qMax(AndroidManager::minimumSDK(kit), minimumNdk);
        // TODO find a way to use the new sysroot layout
        // instead ~/android/ndk-bundle/platforms/android-29/arch-arm64
        // use ~/android/ndk-bundle/toolchains/llvm/prebuilt/linux-x86_64/sysroot
        if (qtVersion) {
            Utils::FilePath sysRoot = AndroidConfigurations::currentConfig().ndkLocation(qtVersion)
                    / "platforms"
                    / QString("android-%1").arg(sdkVersion)
                    / devicePreferredAbi;
            setSysRoot(sysRoot);
            qCDebug(androidDebugSupportLog) << "Sysroot: " << sysRoot;
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

} // namespace Internal
} // namespace Android
