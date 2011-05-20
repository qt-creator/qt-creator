/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "maemoglobal.h"

#include "maemoconstants.h"
#include "maemoqemumanager.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <coreplugin/filemanager.h>
#include <utils/ssh/sshconnection.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/qtversionmanager.h>
#include <qt4projectmanager/qt4target.h>
#include <utils/environment.h>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtGui/QDesktopServices>

namespace Qt4ProjectManager {
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
    return id == QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID);
}

bool MaemoGlobal::isHarmattanTargetId(const QString &id)
{
    return id == QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID);
}

bool MaemoGlobal::isMeegoTargetId(const QString &id)
{
    return id == QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID);
}

bool MaemoGlobal::isValidMaemo5QtVersion(const QString &qmakePath)
{
    return isValidMaemoQtVersion(qmakePath, MaemoDeviceConfig::Maemo5);
}

bool MaemoGlobal::isValidHarmattanQtVersion(const QString &qmakePath)
{
    return isValidMaemoQtVersion(qmakePath, MaemoDeviceConfig::Maemo6);
}

bool MaemoGlobal::isValidMeegoQtVersion(const QString &qmakePath)
{
    return isValidMaemoQtVersion(qmakePath, MaemoDeviceConfig::Meego);
}

bool MaemoGlobal::isLinuxQt(const QtSupport::BaseQtVersion *qtVersion)
{
    if (!qtVersion)
        return false;
    const QList<ProjectExplorer::Abi> &abis = qtVersion->qtAbis();
    foreach (const ProjectExplorer::Abi &abi, abis) {
        if (abi.os() == ProjectExplorer::Abi::LinuxOS)
            return true;
    }
    return false;
}

bool MaemoGlobal::hasLinuxQt(const ProjectExplorer::Target *target)
{
    const Qt4BaseTarget * const qtTarget
        = qobject_cast<const Qt4BaseTarget *>(target);
    if (!qtTarget)
        return false;
    const Qt4BuildConfiguration * const bc
        = qtTarget->activeBuildConfiguration();
    return bc && isLinuxQt(bc->qtVersion());
}

bool MaemoGlobal::isValidMaemoQtVersion(const QString &qmakePath,
    MaemoDeviceConfig::OsVersion maemoVersion)
{
    if (version(qmakePath) != maemoVersion)
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

int MaemoGlobal::applicationIconSize(MaemoDeviceConfig::OsVersion osVersion)
{
    return osVersion == MaemoDeviceConfig::Maemo6 ? 80 : 64;
}

QString MaemoGlobal::remoteSudo(MaemoDeviceConfig::OsVersion osVersion,
    const QString &uname)
{
    if (uname == QLatin1String("root"))
        return QString();
    switch (osVersion) {
    case MaemoDeviceConfig::Maemo5:
    case MaemoDeviceConfig::Maemo6:
    case MaemoDeviceConfig::Meego:
        return devrootshPath();
    default:
        return QString(); // Using sudo would open a can of worms.
    }
}

QString MaemoGlobal::remoteCommandPrefix(MaemoDeviceConfig::OsVersion osVersion,
    const QString &userName, const QString &commandFilePath)
{
    QString prefix = QString::fromLocal8Bit("%1 chmod a+x %2; %3; ")
        .arg(remoteSudo(osVersion, userName), commandFilePath, remoteSourceProfilesCommand());
    if (osVersion != MaemoDeviceConfig::Maemo5 && osVersion != MaemoDeviceConfig::Maemo6)
        prefix += QLatin1String("DISPLAY=:0.0 ");
    return prefix;
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

QString MaemoGlobal::remoteEnvironment(const QList<Utils::EnvironmentItem> &list)
{
    QString env;
    QString placeHolder = QLatin1String("%1=%2 ");
    foreach (const Utils::EnvironmentItem &item, list)
        env.append(placeHolder.arg(item.name).arg(item.value));
    return env.mid(0, env.size() - 1);
}

QString MaemoGlobal::failedToConnectToServerMessage(const Utils::SshConnection::Ptr &connection,
    const MaemoDeviceConfig::ConstPtr &deviceConfig)
{
    QString errorMsg = tr("Could not connect to host: %1")
        .arg(connection->errorString());

    if (deviceConfig->type() == MaemoDeviceConfig::Emulator) {
        if (connection->errorState() == Utils::SshTimeoutError
                || connection->errorState() == Utils::SshSocketError) {
            errorMsg += tr("\nDid you start Qemu?");
        }
   } else if (connection->errorState() == Utils::SshTimeoutError) {
        errorMsg += tr("\nIs the device connected and set up for network access?");
    }
    return errorMsg;
}

QString MaemoGlobal::deviceConfigurationName(const MaemoDeviceConfig::ConstPtr &devConf)
{
    return devConf ? devConf->name() : tr("(No device)");
}

MaemoPortList MaemoGlobal::freePorts(const MaemoDeviceConfig::ConstPtr &devConf,
    const QtSupport::BaseQtVersion *qtVersion)
{
    if (!devConf || !qtVersion)
        return MaemoPortList();
    if (devConf->type() == MaemoDeviceConfig::Emulator) {
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
    return QDir::cleanPath(qmakePath).remove(binQmake);
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

QString MaemoGlobal::madDeveloperUiName(MaemoDeviceConfig::OsVersion osVersion)
{
    return osVersion == MaemoDeviceConfig::Maemo6
        ? tr("SDK Connectivity") : tr("Mad Developer");
}

MaemoDeviceConfig::OsVersion MaemoGlobal::version(const QString &qmakePath)
{
    const QString &name = targetName(qmakePath);
    if (name.startsWith(QLatin1String("fremantle")))
        return MaemoDeviceConfig::Maemo5;
    if (name.startsWith(QLatin1String("harmattan")))
        return MaemoDeviceConfig::Maemo6;
    if (name.startsWith(QLatin1String("meego")))
        return MaemoDeviceConfig::Meego;
    return MaemoDeviceConfig::GenericLinux;
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

bool MaemoGlobal::removeRecursively(const QString &filePath, QString &error)
{
    error.clear();
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists())
        return true;
    QFile::setPermissions(filePath, fileInfo.permissions() | QFile::WriteUser);
    if (fileInfo.isDir()) {
        QDir dir(filePath);
        QStringList fileNames = dir.entryList(QDir::Files | QDir::Hidden
            | QDir::System | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &fileName, fileNames) {
            if (!removeRecursively(filePath + QLatin1Char('/') + fileName, error))
                return false;
        }
        dir.cdUp();
        if (!dir.rmdir(fileInfo.fileName())) {
            error = tr("Failed to remove directory '%1'.")
                .arg(QDir::toNativeSeparators(filePath));
            return false;
        }
    } else {
        if (!QFile::remove(filePath)) {
            error = tr("Failed to remove file '%1'.")
                .arg(QDir::toNativeSeparators(filePath));
            return false;
        }
    }
    return true;
}

bool MaemoGlobal::copyRecursively(const QString &srcFilePath,
    const QString &tgtFilePath, QString *error)
{
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        targetDir.cdUp();
        if (!targetDir.mkdir(QFileInfo(tgtFilePath).fileName())) {
            if (error) {
                *error = tr("Failed to create directory '%1'.")
                    .arg(QDir::toNativeSeparators(tgtFilePath));
                return false;
            }
        }
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &fileName, fileNames) {
            const QString newSrcFilePath
                = srcFilePath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath
                = tgtFilePath + QLatin1Char('/') + fileName;
            if (!copyRecursively(newSrcFilePath, newTgtFilePath))
                return false;
        }
    } else {
        if (!QFile::copy(srcFilePath, tgtFilePath)) {
            if (error) {
                *error = tr("Could not copy file '%1' to '%2'.")
                    .arg(QDir::toNativeSeparators(srcFilePath),
                         QDir::toNativeSeparators(tgtFilePath));
            }
            return false;
        }
    }
    return true;
}

bool MaemoGlobal::isFileNewerThan(const QString &filePath,
    const QDateTime &timeStamp)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || fileInfo.lastModified() >= timeStamp)
        return true;
    if (fileInfo.isDir()) {
        const QStringList dirContents = QDir(filePath)
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &curFileName, dirContents) {
            const QString curFilePath
                = filePath + QLatin1Char('/') + curFileName;
            if (isFileNewerThan(curFilePath, timeStamp))
                return true;
        }
    }
    return false;
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
#ifdef Q_OS_WIN
    Utils::Environment env(proc.systemEnvironment());
    const QString root = maddeRoot(qmakePath);
    env.prependOrSetPath(root + QLatin1String("/bin"));
    env.prependOrSet(QLatin1String("HOME"),
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
    proc.setEnvironment(env.toStringList());
    actualArgs.prepend(command);
    actualCommand = root + QLatin1String("/bin/sh.exe");
#endif
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

QString MaemoGlobal::osVersionToString(MaemoDeviceConfig::OsVersion version)
{
    switch (version) {
    case MaemoDeviceConfig::Maemo5: return QLatin1String("Maemo5/Fremantle");
    case MaemoDeviceConfig::Maemo6: return QLatin1String("Harmattan");
    case MaemoDeviceConfig::Meego: return QLatin1String("Meego");
    case MaemoDeviceConfig::GenericLinux: return QLatin1String("Other Linux");
    }
    qDebug("%s: Unknown OS Version %d.", Q_FUNC_INFO, version);
    return QString();
}

MaemoGlobal::PackagingSystem MaemoGlobal::packagingSystem(MaemoDeviceConfig::OsVersion osVersion)
{
    switch (osVersion) {
    case MaemoDeviceConfig::Maemo5: case MaemoDeviceConfig::Maemo6: return Dpkg;
    case MaemoDeviceConfig::Meego: return Rpm;
    case MaemoDeviceConfig::GenericLinux: return Tar;
    default: qFatal("%s: Missing case in switch.", Q_FUNC_INFO);
    }
    return static_cast<PackagingSystem>(-1);
}

} // namespace Internal
} // namespace Qt4ProjectManager
