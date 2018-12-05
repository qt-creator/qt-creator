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

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
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
Q_LOGGING_CATEGORY(androidDebugSupportLog, "qtc.android.run.androiddebugsupport", QtWarningMsg)
}

using namespace Debugger;
using namespace ProjectExplorer;

namespace Android {
namespace Internal {

static const char * const qMakeVariables[] = {
         "QT_INSTALL_LIBS",
         "QT_INSTALL_PLUGINS",
         "QT_INSTALL_QML"
};

static QStringList qtSoPaths(QtSupport::BaseQtVersion *qtVersion)
{
    if (!qtVersion)
        return QStringList();

    QSet<QString> paths;
    for (uint i = 0; i < sizeof qMakeVariables / sizeof qMakeVariables[0]; ++i) {
        QString path = qtVersion->qmakeProperty(qMakeVariables[i]);
        if (path.isNull())
            continue;
        QDirIterator it(path, QStringList("*.so"), QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            paths.insert(it.fileInfo().absolutePath());
        }
    }
    return paths.toList();
}

static QStringList uniquePaths(const QStringList &files)
{
    QSet<QString> paths;
    foreach (const QString &file, files)
        paths<<QFileInfo(file).absolutePath();
    return paths.toList();
}

static QStringList getSoLibSearchPath(const RunConfiguration *rc)
{
    Target *target = rc->target();
    const ProjectNode *node = target->project()->findNodeForBuildKey(rc->buildKey());
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

static QStringList getExtraLibs(const RunConfiguration *rc)
{
    const ProjectNode *node = rc->target()->project()->findNodeForBuildKey(rc->buildKey());
    if (!node)
        return {};

    return node->data(Android::Constants::AndroidExtraLibs).toStringList();
}

static QString toNdkArch(const QString &arch)
{
    if (arch == QLatin1String("armeabi-v7a") || arch == QLatin1String("armeabi"))
        return QLatin1String("arch-arm");
    if (arch == QLatin1String("arm64-v8a"))
        return QLatin1String("arch-arm64");
    return QLatin1String("arch-") + arch;
}

AndroidDebugSupport::AndroidDebugSupport(RunControl *runControl, const QString &intentName)
    : Debugger::DebuggerRunTool(runControl)
{
    setId("AndroidDebugger");
    m_runner = new AndroidRunner(runControl, intentName);
    addStartDependency(m_runner);
}

void AndroidDebugSupport::start()
{
    auto runConfig = runControl()->runConfiguration();
    Target *target = runConfig->target();
    Kit *kit = target->kit();

    setStartMode(AttachToRemoteServer);
    const QString packageName = AndroidManager::packageName(target);
    setRunControlName(packageName);
    setUseContinueInsteadOfRun(true);
    setAttachPid(m_runner->pid());

    qCDebug(androidDebugSupportLog) << "Start. Package name: " << packageName
                                    << "PID: " << m_runner->pid().pid();
    if (!Utils::HostOsInfo::isWindowsHost() &&
            AndroidConfigurations::currentConfig().ndkVersion() >= QVersionNumber(11, 0, 0)) {
        qCDebug(androidDebugSupportLog) << "UseTargetAsync: " << true;
        setUseTargetAsync(true);
    }

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(kit);

    if (isCppDebugging()) {
        qCDebug(androidDebugSupportLog) << "C++ debugging enabled";
        QStringList solibSearchPath = getSoLibSearchPath(runConfig);
        QStringList extraLibs = getExtraLibs(runConfig);
        solibSearchPath.append(qtSoPaths(qtVersion));
        solibSearchPath.append(uniquePaths(extraLibs));
        setSolibSearchPath(solibSearchPath);
        qCDebug(androidDebugSupportLog) << "SoLibSearchPath: "<<solibSearchPath;
        setSymbolFile(target->activeBuildConfiguration()->buildDirectory().toString()
                      + "/app_process");
        setSkipExecutableValidation(true);
        setUseExtendedRemote(true);
        QUrl gdbServer;
        gdbServer.setHost(QHostAddress(QHostAddress::LocalHost).toString());
        gdbServer.setPort(m_runner->gdbServerPort().number());
        setRemoteChannel(gdbServer);

        int sdkVersion = qMax(AndroidManager::minimumSDK(target->kit()),
                              AndroidManager::minimumNDK(target->kit()));
        Utils::FileName sysRoot = AndroidConfigurations::currentConfig().ndkLocation()
                .appendPath("platforms")
                .appendPath(QString("android-%1").arg(sdkVersion))
                .appendPath(toNdkArch(AndroidManager::targetArch(target)));
        setSysRoot(sysRoot);
        qCDebug(androidDebugSupportLog) << "Sysroot: " << sysRoot;
    }
    if (isQmlDebugging()) {
        qCDebug(androidDebugSupportLog) << "QML debugging enabled. QML server: "
                                        << m_runner->qmlServer().toDisplayString();
        setQmlServer(m_runner->qmlServer());
        //TODO: Not sure if these are the right paths.
        if (qtVersion)
            addSearchDirectory(qtVersion->qmlPath());
    }

    DebuggerRunTool::start();
}

void AndroidDebugSupport::stop()
{
    qCDebug(androidDebugSupportLog) << "Stop";
    DebuggerRunTool::stop();
}

} // namespace Internal
} // namespace Android
