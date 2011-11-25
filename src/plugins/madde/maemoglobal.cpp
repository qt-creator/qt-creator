/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "maemoglobal.h"

#include "maemoconstants.h"
#include "maemoqemumanager.h"

#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/baseqtversion.h>
#include <remotelinux/linuxdeviceconfiguration.h>
#include <remotelinux/remotelinux_constants.h>
#include <utils/environment.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtGui/QDesktopServices>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Constants;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {
namespace {
static const QLatin1String binQmake("/bin/qmake" EXEC_SUFFIX);
}

bool MaemoGlobal::isMaemoTargetId(const QString &id)
{
    return isFremantleTargetId(id) || isHarmattanTargetId(id)
        || isMeegoTargetId(id);
}

bool MaemoGlobal::isFremantleTargetId(const QString &id)
{
    return id == QLatin1String(MAEMO5_DEVICE_TARGET_ID);
}

bool MaemoGlobal::isHarmattanTargetId(const QString &id)
{
    return id == QLatin1String(HARMATTAN_DEVICE_TARGET_ID);
}

bool MaemoGlobal::isMeegoTargetId(const QString &id)
{
    return id == QLatin1String(MEEGO_DEVICE_TARGET_ID);
}

bool MaemoGlobal::isValidMaemo5QtVersion(const QString &qmakePath)
{
    return isValidMaemoQtVersion(qmakePath, QLatin1String(Maemo5OsType));
}

bool MaemoGlobal::isValidHarmattanQtVersion(const QString &qmakePath)
{
    return isValidMaemoQtVersion(qmakePath, QLatin1String(HarmattanOsType));
}

bool MaemoGlobal::isValidMeegoQtVersion(const QString &qmakePath)
{
    return isValidMaemoQtVersion(qmakePath, QLatin1String(MeeGoOsType));
}

bool MaemoGlobal::isValidMaemoQtVersion(const QString &qmakePath, const QString &osType)
{
    if (MaemoGlobal::osType(qmakePath) != osType)
        return false;
    QProcess madAdminProc;
    const QStringList arguments(QLatin1String("list"));
    if (!callMadAdmin(madAdminProc, arguments, qmakePath, false))
        return false;
    if (!madAdminProc.waitForStarted() || !madAdminProc.waitForFinished())
        return false;

    madAdminProc.setReadChannel(QProcess::StandardOutput);
    const QByteArray tgtName = targetName(qmakePath).toAscii();
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

int MaemoGlobal::applicationIconSize(const QString &osType)
{
    return osType == QLatin1String(HarmattanOsType) ? 80 : 64;
}

QString MaemoGlobal::remoteSudo(const QString &osType, const QString &uname)
{
    if (uname == QLatin1String("root"))
        return QString();
    if (osType == QLatin1String(Maemo5OsType) || osType == QLatin1String(HarmattanOsType)
            || osType == QLatin1String(MeeGoOsType)) {
        return devrootshPath();
    }
    return QString(); // Using sudo would open a can of worms.
}

QString MaemoGlobal::remoteSourceProfilesCommand()
{
    const QList<QByteArray> profiles = QList<QByteArray>() << "/etc/profile"
        << "/home/user/.profile" << "~/.profile";
    QByteArray remoteCall(":");
    foreach (const QByteArray &profile, profiles)
        remoteCall += "; test -f " + profile + " && source " + profile;
    return QString::fromAscii(remoteCall);
}

PortList MaemoGlobal::freePorts(const LinuxDeviceConfiguration::ConstPtr &devConf,
    const QtSupport::BaseQtVersion *qtVersion)
{
    if (!devConf || !qtVersion)
        return PortList();
    if (devConf->deviceType() == LinuxDeviceConfiguration::Emulator) {
        MaemoQemuRuntime rt;
        const int id = qtVersion->uniqueId();
        if (MaemoQemuManager::instance().runtimeForQtVersion(id, &rt))
            return rt.m_freePorts;
    }
    return devConf->freePorts();
}

QString MaemoGlobal::maddeRoot(const QString &qmakePath)
{
    QDir dir(targetRoot(qmakePath));
    dir.cdUp(); dir.cdUp();
    return dir.absolutePath();
}

QString MaemoGlobal::targetRoot(const QString &qmakePath)
{
#ifdef Q_OS_WIN
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    return QDir::cleanPath(qmakePath).remove(binQmake, cs);
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

QString MaemoGlobal::madDeveloperUiName(const QString &osType)
{
    return osType == QLatin1String(HarmattanOsType)
        ? tr("SDK Connectivity") : tr("Mad Developer");
}

QString MaemoGlobal::osType(const QString &qmakePath)
{
    const QString &name = targetName(qmakePath);
    if (name.startsWith(QLatin1String("fremantle")))
        return QLatin1String(Maemo5OsType);
    if (name.startsWith(QLatin1String("harmattan")))
        return QLatin1String(HarmattanOsType);
    if (name.startsWith(QLatin1String("meego")))
        return QLatin1String(MeeGoOsType);
    return QLatin1String(RemoteLinux::Constants::GenericLinuxOsType);
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

void MaemoGlobal::addMaddeEnvironment(Utils::Environment &env, const QString &qmakePath)
{
    Utils::Environment maddeEnv;
#ifdef Q_OS_WIN
    const QString root = maddeRoot(qmakePath);
    env.prependOrSetPath(root + QLatin1String("/bin"));
    env.prependOrSet(QLatin1String("HOME"),
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
#else
    Q_UNUSED(qmakePath);
#endif
    for (Utils::Environment::const_iterator it = maddeEnv.constBegin(); it != maddeEnv.constEnd(); ++it)
        env.prependOrSet(it.key(), it.value());
}

void MaemoGlobal::transformMaddeCall(QString &command, QStringList &args, const QString &qmakePath)
{
#ifdef Q_OS_WIN
    const QString root = maddeRoot(qmakePath);
    args.prepend(command);
    command = root + QLatin1String("/bin/sh.exe");
#else
    Q_UNUSED(command);
    Q_UNUSED(args);
    Q_UNUSED(qmakePath);
#endif
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
    Utils::Environment env(proc.systemEnvironment());
    addMaddeEnvironment(env, qmakePath);
    proc.setEnvironment(env.toStringList());
    transformMaddeCall(actualCommand, actualArgs, qmakePath);
    proc.start(actualCommand, actualArgs);
    return true;
}

QStringList MaemoGlobal::targetArgs(const QString &qmakePath, bool useTarget)
{
    QStringList args;
    if (useTarget) {
        args << QLatin1String("-t") << targetName(qmakePath);
    }
    return args;
}

} // namespace Internal
} // namespace Madde
