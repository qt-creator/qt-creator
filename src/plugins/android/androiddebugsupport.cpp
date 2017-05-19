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

#include "androidglobal.h"
#include "androidrunner.h"
#include "androidmanager.h"
#include "androidqtsupport.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/hostosinfo.h>

#include <QDirIterator>

using namespace Debugger;
using namespace ProjectExplorer;

namespace Android {
namespace Internal {

static const char * const qMakeVariables[] = {
         "QT_INSTALL_LIBS",
         "QT_INSTALL_PLUGINS",
         "QT_INSTALL_IMPORTS"
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

static QString toNdkArch(const QString &arch)
{
    if (arch == QLatin1String("armeabi-v7a") || arch == QLatin1String("armeabi"))
        return QLatin1String("arch-arm");
    if (arch == QLatin1String("arm64-v8a"))
        return QLatin1String("arch-arm64");
    return QLatin1String("arch-") + arch;
}

AndroidDebugSupport::AndroidDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
{
    setDisplayName("AndroidDebugger");
    m_runner = new AndroidRunner(runControl);
    addDependency(m_runner);
}

void AndroidDebugSupport::start()
{
    auto runConfig = runControl()->runConfiguration();
    Target *target = runConfig->target();
    Kit *kit = target->kit();

    DebuggerStartParameters params;
    params.startMode = AttachToRemoteServer;
    params.displayName = AndroidManager::packageName(target);
    params.useContinueInsteadOfRun = true;
    params.attachPID = m_runner->pid();
    if (!Utils::HostOsInfo::isWindowsHost() &&
            AndroidConfigurations::currentConfig().ndkVersion() >= QVersionNumber(11, 0, 0)) {
        params.useTargetAsync = true;
    }

    if (isCppDebugging()) {
        Utils::Port gdbServerPort = m_runner->gdbServerPort();
        params.symbolFile = target->activeBuildConfiguration()->buildDirectory().toString() + "/app_process";
        params.skipExecutableValidation = true;
        params.useExtendedRemote = true;
        params.remoteChannel = ":" + gdbServerPort.toString();
        params.solibSearchPath = AndroidManager::androidQtSupport(target)->soLibSearchPath(target);
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
        params.solibSearchPath.append(qtSoPaths(version));
        params.solibSearchPath.append(uniquePaths(AndroidManager::androidQtSupport(target)->androidExtraLibs(target)));
        params.sysRoot = AndroidConfigurations::currentConfig().ndkLocation().appendPath("platforms")
                                                     .appendPath(QString("android-%1").arg(AndroidManager::minimumSDK(target)))
                                                     .appendPath(toNdkArch(AndroidManager::targetArch(target))).toString();
    }
    if (isQmlDebugging()) {
        params.qmlServer.port = m_runner->qmlServerPort();
        //TODO: Not sure if these are the right paths.
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
        if (version) {
            const QString qmlQtDir = version->qmakeProperty("QT_INSTALL_QML");
            params.additionalSearchDirectories = QStringList(qmlQtDir);
        }
    }

    setStartParameters(params);

    // FIXME: Move signal to base class and generalize handling.
    connect(this, &DebuggerRunTool::aboutToNotifyInferiorSetupOk,
            m_runner, &AndroidRunner::remoteDebuggerRunning);

    DebuggerRunTool::start();
}

void AndroidDebugSupport::stop()
{
    DebuggerRunTool::stop();
}

} // namespace Internal
} // namespace Android
