/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef CODACLIENTAPPLICATION_H
#define CODACLIENTAPPLICATION_H

#include <QCoreApplication>
#include <QStringList>
#include <QSharedPointer>
#include <QTime>

QT_FORWARD_DECLARE_CLASS(QFile)

namespace Coda {
    struct CodaCommandResult;
    class CodaDevice;
    class CodaEvent;
}

class CodaClientApplication : public QCoreApplication
{
    Q_OBJECT
public:
    enum Mode { Invalid, Ping, Launch, Put, Stat, Install, Uninstall };

    explicit CodaClientApplication(int &argc, char **argv);
    ~CodaClientApplication();

    enum ParseArgsResult { ParseArgsOk, ParseArgsError, ParseInitError, ParseArgsHelp };
    ParseArgsResult parseArguments(QString *errorMessage);

    bool start();

    static QString usage();

private slots:
    void slotError(const QString &);
    void slotTrkLogMessage(const QString &);
    void slotCodaEvent(const Coda::CodaEvent &);
    void slotSerialPong(const QString &);

private:
    void printTimeStamp();
    bool parseArgument(const QString &a, int argNumber, QString *errorMessage);
    void handleCreateProcess(const Coda::CodaCommandResult &result);
    void handleFileSystemOpen(const Coda::CodaCommandResult &result);
    void handleFileSystemWrite(const Coda::CodaCommandResult &result);
    void handleFileSystemClose(const Coda::CodaCommandResult &result);
    void handleFileSystemFStat(const Coda::CodaCommandResult &result);
    void handleSymbianInstall(const Coda::CodaCommandResult &result);
    void handleUninstall(const Coda::CodaCommandResult &result);
    void doExit(int ex);
    void putSendNextChunk();
    void closeRemoteFile();

    Mode m_mode;
    QString m_address;
    unsigned short m_port;
    QString m_launchBinary;
    QStringList m_launchArgs;
    unsigned m_launchUID;
    bool m_launchDebug;
    unsigned m_uninstallPackage;
    QString m_installSisFile;
    QString m_installTargetDrive;
    bool m_installSilently;
    QString m_putLocalFile;
    QString m_putRemoteFile;
    bool m_putWriteOk;
    bool m_statFstatOk;
    QScopedPointer<QFile> m_putFile;
    quint64 m_putLastChunkSize;
    QString m_statRemoteFile;
    QByteArray m_remoteFileHandle;
    quint64 m_putChunkSize;
    unsigned m_verbose;
    QTime m_startTime;
    QScopedPointer<Coda::CodaDevice> m_trkDevice;
};

#endif // CODACLIENTAPPLICATION_H
