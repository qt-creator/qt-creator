/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "maemopackageinstaller.h"

#include "maemoglobal.h"

#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshremoteprocessrunner.h>

using namespace Utils;

namespace Qt4ProjectManager {
namespace Internal {

AbstractMaemoPackageInstaller::AbstractMaemoPackageInstaller(QObject *parent)
    : QObject(parent), m_isRunning(false)
{
}

AbstractMaemoPackageInstaller::~AbstractMaemoPackageInstaller() {}

void AbstractMaemoPackageInstaller::installPackage(const SshConnection::Ptr &connection,
    const QString &packageFilePath, bool removePackageFile)
{
    Q_ASSERT(connection && connection->state() == SshConnection::Connected);
    Q_ASSERT(!m_isRunning);

    prepareInstallation();
    m_installer = SshRemoteProcessRunner::create(connection);
    connect(m_installer.data(), SIGNAL(connectionError(Utils::SshError)),
        SLOT(handleConnectionError()));
    connect(m_installer.data(), SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleInstallerOutput(QByteArray)));
    connect(m_installer.data(), SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleInstallerErrorOutput(QByteArray)));
    connect(m_installer.data(), SIGNAL(processClosed(int)),
        SLOT(handleInstallationFinished(int)));

    const QString space = QLatin1String(" ");
    QString cmdLine = MaemoGlobal::remoteSudo() + space + installCommand()
        + space + installCommandArguments().join(space) + space
        + packageFilePath;
    if (removePackageFile) {
        cmdLine += QLatin1String(" && (rm ") + packageFilePath
            + QLatin1String(" || :)");
    }
    m_installer->run(cmdLine.toUtf8());
    m_isRunning = true;
}

void AbstractMaemoPackageInstaller::cancelInstallation()
{
    Q_ASSERT(m_isRunning);
    const SshRemoteProcessRunner::Ptr killProcess
        = SshRemoteProcessRunner::create(m_installer->connection());
    killProcess->run("pkill " + installCommand().toUtf8());
    setFinished();
}

void AbstractMaemoPackageInstaller::handleConnectionError()
{
    if (!m_isRunning)
        return;
    emit finished(tr("Connection failure: %1")
        .arg(m_installer->connection()->errorString()));
    setFinished();
}

void AbstractMaemoPackageInstaller::handleInstallationFinished(int exitStatus)
{
    if (!m_isRunning)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally
            || m_installer->process()->exitCode() != 0) {
        emit finished(tr("Installing package failed."));
    } else if (!errorString().isEmpty()) {
        emit finished(errorString());
    } else {
        emit finished();
    }

    setFinished();
}

void AbstractMaemoPackageInstaller::handleInstallerOutput(const QByteArray &output)
{
    emit stdout(QString::fromUtf8(output));
}

void AbstractMaemoPackageInstaller::handleInstallerErrorOutput(const QByteArray &output)
{
    emit stderr(QString::fromUtf8(output));
}

void AbstractMaemoPackageInstaller::setFinished()
{
    disconnect(m_installer.data(), 0, this, 0);
    m_installer.clear();
    m_isRunning = false;
}


MaemoDebianPackageInstaller::MaemoDebianPackageInstaller(QObject *parent)
    : AbstractMaemoPackageInstaller(parent)
{
    connect(this, SIGNAL(stderr(QString)),
        SLOT(handleInstallerErrorOutput(QString)));
}

void MaemoDebianPackageInstaller::prepareInstallation()
{
    m_installerStderr.clear();
}

QString MaemoDebianPackageInstaller::installCommand() const
{
    return QLatin1String("dpkg");
}

QStringList MaemoDebianPackageInstaller::installCommandArguments() const
{
    return QStringList() << QLatin1String("-i")
        << QLatin1String("--no-force-downgrade");
}

void MaemoDebianPackageInstaller::handleInstallerErrorOutput(const QString &output)
{
    m_installerStderr += output;
}

QString MaemoDebianPackageInstaller::errorString() const
{
    if (m_installerStderr.contains(QLatin1String("Will not downgrade"))) {
        return tr("Installation failed: "
            "You tried to downgrade a package, which is not allowed.");
    } else {
        return QString();
    }
}


MaemoRpmPackageInstaller::MaemoRpmPackageInstaller(QObject *parent)
    : AbstractMaemoPackageInstaller(parent)
{
}

QString MaemoRpmPackageInstaller::installCommand() const
{
    return QLatin1String("rpm");
}

QStringList MaemoRpmPackageInstaller::installCommandArguments() const
{
    return QStringList() << QLatin1String("-Uhv");
}


MaemoTarPackageInstaller::MaemoTarPackageInstaller(QObject *parent)
    : AbstractMaemoPackageInstaller(parent)
{
}

QString MaemoTarPackageInstaller::installCommand() const
{
    return QLatin1String("tar");
}

QStringList MaemoTarPackageInstaller::installCommandArguments() const
{
    return QStringList() << QLatin1String("xvf");
}

} // namespace Internal
} // namespace Qt4ProjectManager
