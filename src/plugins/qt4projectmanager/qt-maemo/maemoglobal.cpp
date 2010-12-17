/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemoglobal.h"

#include "maemoconstants.h"
#include "maemodeviceconfigurations.h"

#include <coreplugin/ssh/sshconnection.h>
#include <utils/environment.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QDesktopServices>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QString>

#define TR(text) QCoreApplication::translate("Qt4ProjectManager::Internal::MaemoGlobal", text)

namespace Qt4ProjectManager {
namespace Internal {
namespace {
static const QLatin1String binQmake("/bin/qmake" EXEC_SUFFIX);
}

QString MaemoGlobal::homeDirOnDevice(const QString &uname)
{
    return uname == QLatin1String("root")
        ? QString::fromLatin1("/root")
        : QLatin1String("/home/") + uname;
}

QString MaemoGlobal::remoteSudo()
{
    return QLatin1String("/usr/lib/mad-developer/devrootsh");
}

QString MaemoGlobal::remoteCommandPrefix(const QString &commandFilePath)
{
    return QString::fromLocal8Bit("%1 chmod a+x %2; %3; ")
        .arg(remoteSudo(), commandFilePath, remoteSourceProfilesCommand());
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

QString MaemoGlobal::failedToConnectToServerMessage(const Core::SshConnection::Ptr &connection,
    const MaemoDeviceConfig &deviceConfig)
{
    QString errorMsg = TR("Could not connect to host: %1")
        .arg(connection->errorString());

    if (deviceConfig.type == MaemoDeviceConfig::Simulator) {
        if (connection->errorState() == Core::SshTimeoutError
                || connection->errorState() == Core::SshSocketError) {
            errorMsg += TR("\nDid you start Qemu?");
        }
    } else if (connection->errorState() == Core::SshTimeoutError) {
        errorMsg += TR("\nIs the device connected and set up for network access?");
    }
    return errorMsg;
}

QString MaemoGlobal::maddeRoot(const QString &qmakePath)
{
    QDir dir(QDir::cleanPath(qmakePath).remove(binQmake));
    dir.cdUp(); dir.cdUp();
    return dir.absolutePath();
}

QString MaemoGlobal::targetName(const QString &qmakePath)
{
    const QString target = QDir::cleanPath(qmakePath).remove(binQmake);
    return target.mid(target.lastIndexOf(QLatin1Char('/')) + 1);
}

bool MaemoGlobal::removeRecursively(const QString &filePath, QString &error)
{
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
            error = TR("Failed to remove directory '%1'.")
                .arg(QDir::toNativeSeparators(filePath));
            return false;
        }
    } else {
        if (!QFile::remove(filePath)) {
            error = TR("Failed to remove file '%1'.")
                .arg(QDir::toNativeSeparators(filePath));
            return false;
        }
    }
    return true;
}

void MaemoGlobal::callMaddeShellScript(QProcess &proc, const QString &maddeRoot,
    const QString &command, const QStringList &args)
{
    QString actualCommand = command;
    QStringList actualArgs = args;
#ifdef Q_OS_WIN
    Utils::Environment env(proc.systemEnvironment());
    env.prependOrSetPath(maddeRoot + QLatin1String("/bin"));
    env.prependOrSet(QLatin1String("HOME"),
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
    proc.setEnvironment(env.toStringList());
    actualArgs.prepend(command);
    actualCommand = maddeRoot + QLatin1String("/bin/sh.exe");
#else
    Q_UNUSED(maddeRoot);
#endif
    proc.start(actualCommand, actualArgs);
}

} // namespace Internal
} // namespace Qt4ProjectManager
