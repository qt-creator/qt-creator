/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "maemoglobal.h"

#include "maemoconstants.h"
#include "maemoqemumanager.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <remotelinux/remotelinux_constants.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QDesktopServices>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Constants;
using namespace RemoteLinux;
using namespace Utils;

namespace Madde {
namespace Internal {
static const QString binQmake = QLatin1String("/bin/qmake" QTC_HOST_EXE_SUFFIX);

bool MaemoGlobal::hasMaemoDevice(const Kit *k)
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (dev.isNull())
        return false;

    const Core::Id type = dev->type();
    return type == Maemo5OsType || type == HarmattanOsType;
}

bool MaemoGlobal::supportsMaemoDevice(const Kit *k)
{
    const Core::Id type = DeviceTypeKitInformation::deviceTypeId(k);
    return type == Maemo5OsType || type == HarmattanOsType;
}

bool MaemoGlobal::isValidMaemo5QtVersion(const QString &qmakePath)
{
    return isValidMaemoQtVersion(qmakePath, Core::Id(Maemo5OsType));
}

bool MaemoGlobal::isValidHarmattanQtVersion(const QString &qmakePath)
{
    return isValidMaemoQtVersion(qmakePath, Core::Id(HarmattanOsType));
}

bool MaemoGlobal::isValidMaemoQtVersion(const QString &qmakePath, Core::Id deviceType)
{
    if (MaemoGlobal::deviceType(qmakePath) != deviceType)
        return false;
    QProcess madAdminProc;
    const QStringList arguments(QLatin1String("list"));
    if (!callMadAdmin(madAdminProc, arguments, qmakePath, false))
        return false;
    if (!madAdminProc.waitForStarted() || !madAdminProc.waitForFinished())
        return false;

    madAdminProc.setReadChannel(QProcess::StandardOutput);
    const QByteArray tgtName = targetName(qmakePath).toLatin1();
    while (madAdminProc.canReadLine()) {
        const QByteArray &line = madAdminProc.readLine();
        if (line.contains(tgtName)
            && (line.contains("(installed)") || line.contains("(default)")))
            return true;
    }
    return false;
}


QString MaemoGlobal::homeDirOnDevice(const QString &uname)
{
    return uname == QLatin1String("root")
        ? QString::fromLatin1("/root")
        : QLatin1String("/home/") + uname;
}

QString MaemoGlobal::devrootshPath()
{
    return QLatin1String("/usr/lib/mad-developer/devrootsh");
}

int MaemoGlobal::applicationIconSize(const Target *target)
{
    Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(target->kit());
    return deviceType == HarmattanOsType ? 80 : 64;
}

QString MaemoGlobal::remoteSudo(Core::Id deviceType, const QString &uname)
{
    if (uname == QLatin1String("root"))
        return QString();
    if (deviceType == Maemo5OsType || deviceType == HarmattanOsType)
        return devrootshPath();
    return QString(); // Using sudo would open a can of worms.
}

QString MaemoGlobal::remoteSourceProfilesCommand()
{
    const QList<QByteArray> profiles = QList<QByteArray>() << "/etc/profile"
        << "/home/user/.profile" << "~/.profile";
    QByteArray remoteCall(":");
    foreach (const QByteArray &profile, profiles)
        remoteCall += "; test -f " + profile + " && source " + profile;
    return QString::fromLatin1(remoteCall);
}

PortList MaemoGlobal::freePorts(const Kit *k)
{
    IDevice::ConstPtr device = DeviceKitInformation::device(k);
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);

    if (!device || !qtVersion)
        return PortList();
    if (device->machineType() == IDevice::Emulator) {
        MaemoQemuRuntime rt;
        const int id = qtVersion->uniqueId();
        if (MaemoQemuManager::instance().runtimeForQtVersion(id, &rt))
            return rt.m_freePorts;
    }
    return device->freePorts();
}

QString MaemoGlobal::maddeRoot(const QString &qmakePath)
{
    QDir dir(targetRoot(qmakePath));
    dir.cdUp(); dir.cdUp();
    return dir.absolutePath();
}

FileName MaemoGlobal::maddeRoot(const Kit *k)
{
    return SysRootKitInformation::sysRoot(k).parentDir().parentDir();
}

QString MaemoGlobal::targetRoot(const QString &qmakePath)
{
    return QDir::cleanPath(qmakePath).remove(binQmake, HostOsInfo::fileNameCaseSensitivity());
}

QString MaemoGlobal::targetName(const QString &qmakePath)
{
    return QDir(targetRoot(qmakePath)).dirName();
}

QString MaemoGlobal::madAdminCommand(const QString &qmakePath)
{
    return maddeRoot(qmakePath) + QLatin1String("/bin/mad-admin");
}

QString MaemoGlobal::madCommand(const QString &qmakePath)
{
    return maddeRoot(qmakePath) + QLatin1String("/bin/mad");
}

QString MaemoGlobal::madDeveloperUiName(Core::Id deviceType)
{
    return deviceType == HarmattanOsType
        ? tr("SDK Connectivity") : tr("Mad Developer");
}

Core::Id MaemoGlobal::deviceType(const QString &qmakePath)
{
    const QString &name = targetName(qmakePath);
    if (name.startsWith(QLatin1String("fremantle")))
        return Core::Id(Maemo5OsType);
    if (name.startsWith(QLatin1String("harmattan")))
        return Core::Id(HarmattanOsType);
    return Core::Id(RemoteLinux::Constants::GenericLinuxOsType);
}

QString MaemoGlobal::architecture(const QString &qmakePath)
{
    QProcess proc;
    const QStringList args = QStringList() << QLatin1String("uname")
        << QLatin1String("-m");
    if (!callMad(proc, args, qmakePath, true))
        return QString();
    if (!proc.waitForFinished())
        return QString();
    QString arch = QString::fromUtf8(proc.readAllStandardOutput());
    arch.chop(1); // Newline
    return arch;
}

void MaemoGlobal::addMaddeEnvironment(Environment &env, const QString &qmakePath)
{
    Environment maddeEnv;
    if (HostOsInfo::isWindowsHost()) {
        const QString root = maddeRoot(qmakePath);
        env.prependOrSetPath(root + QLatin1String("/bin"));
        env.prependOrSet(QLatin1String("HOME"),
                QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
    }
    for (Environment::const_iterator it = maddeEnv.constBegin(); it != maddeEnv.constEnd(); ++it)
        env.prependOrSet(it.key(), it.value());
}

void MaemoGlobal::transformMaddeCall(QString &command, QStringList &args, const QString &qmakePath)
{
    if (HostOsInfo::isWindowsHost()) {
        const QString root = maddeRoot(qmakePath);
        args.prepend(command);
        command = root + QLatin1String("/bin/sh.exe");
    }
}

bool MaemoGlobal::callMad(QProcess &proc, const QStringList &args,
    const QString &qmakePath, bool useTarget)
{
    return callMaddeShellScript(proc, qmakePath, madCommand(qmakePath), args,
        useTarget);
}

bool MaemoGlobal::callMadAdmin(QProcess &proc, const QStringList &args,
    const QString &qmakePath, bool useTarget)
{
    return callMaddeShellScript(proc, qmakePath, madAdminCommand(qmakePath),
        args, useTarget);
}

bool MaemoGlobal::callMaddeShellScript(QProcess &proc,
    const QString &qmakePath, const QString &command, const QStringList &args,
    bool useTarget)
{
    if (!QFileInfo(command).exists())
        return false;
    QString actualCommand = command;
    QStringList actualArgs = targetArgs(qmakePath, useTarget) + args;
    Environment env(proc.systemEnvironment());
    addMaddeEnvironment(env, qmakePath);
    proc.setEnvironment(env.toStringList());
    transformMaddeCall(actualCommand, actualArgs, qmakePath);
    proc.start(actualCommand, actualArgs);
    return true;
}

QStringList MaemoGlobal::targetArgs(const QString &qmakePath, bool useTarget)
{
    QStringList args;
    if (useTarget)
        args << QLatin1String("-t") << targetName(qmakePath);
    return args;
}

} // namespace Internal
} // namespace Madde
