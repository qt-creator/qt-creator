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

#ifndef ABSTRACTMAEMOPACKAGEINSTALLER_H
#define ABSTRACTMAEMOPACKAGEINSTALLER_H

namespace Utils {
class SshConnection;
class SshRemoteProcessRunner;
}

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace RemoteLinux {
class LinuxDeviceConfiguration;

namespace Internal {

class AbstractMaemoPackageInstaller : public QObject
{
    Q_OBJECT
public:
    ~AbstractMaemoPackageInstaller();

    void installPackage(const QSharedPointer<Utils::SshConnection> &connection,
        const QSharedPointer<const LinuxDeviceConfiguration> &devConfig,
        const QString &packageFilePath, bool removePackageFile);
    void cancelInstallation();

signals:
    void stdoutData(const QString &output);
    void stderrData(const QString &output);
    void finished(const QString &errorMsg = QString());

protected:
    explicit AbstractMaemoPackageInstaller(QObject *parent = 0);
    bool isRunning() const { return m_isRunning; }

private slots:
    void handleConnectionError();
    void handleInstallationFinished(int exitStatus);
    void handleInstallerOutput(const QByteArray &output);
    void handleInstallerErrorOutput(const QByteArray &output);

private:
    virtual void prepareInstallation() {}
    virtual QString workingDirectory() const { return QLatin1String("/tmp"); }
    virtual QString installCommand() const=0;
    virtual QStringList installCommandArguments() const=0;
    virtual QString errorString() const { return QString(); }
    void setFinished();

    bool m_isRunning;
    QSharedPointer<Utils::SshRemoteProcessRunner> m_installer;
};


class MaemoDebianPackageInstaller: public AbstractMaemoPackageInstaller
{
    Q_OBJECT
public:
    MaemoDebianPackageInstaller(QObject *parent);

private slots:
    virtual void prepareInstallation();
    virtual QString installCommand() const;
    virtual QStringList installCommandArguments() const;
    virtual QString errorString() const;
    void handleInstallerErrorOutput(const QString &output);

private:
    QString m_installerStderr;
};


class MaemoRpmPackageInstaller : public AbstractMaemoPackageInstaller
{
    Q_OBJECT
public:
    MaemoRpmPackageInstaller(QObject *parent);

private:
    virtual QString installCommand() const;
    virtual QStringList installCommandArguments() const;
};


class MaemoTarPackageInstaller : public AbstractMaemoPackageInstaller
{
    Q_OBJECT
public:
    MaemoTarPackageInstaller(QObject *parent);

private:
    virtual QString installCommand() const;
    virtual QStringList installCommandArguments() const;
    virtual QString workingDirectory() const { return QLatin1String("/"); }
};


} // namespace Internal
} // namespace RemoteLinux

#endif // ABSTRACTMAEMOPACKAGEINSTALLER_H
