/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CODACLIENTAPPLICATION_H
#define CODACLIENTAPPLICATION_H

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QTime>

QT_FORWARD_DECLARE_CLASS(QFile)

namespace tcftrk {
    struct TcfTrkCommandResult;
    class TcfTrkDevice;
    class TcfTrkEvent;
}

class CodaClientApplication : public QCoreApplication
{
    Q_OBJECT
public:
    enum Mode { Invalid, Ping, Launch, Put, Stat, Install };

    explicit CodaClientApplication(int &argc, char **argv);
    ~CodaClientApplication();

    enum ParseArgsResult { ParseArgsOk, ParseArgsError, ParseInitError, ParseArgsHelp };
    ParseArgsResult parseArguments(QString *errorMessage);

    bool start();

    static QString usage();

private slots:
    void slotError(const QString &);
    void slotTrkLogMessage(const QString &);
    void slotTcftrkEvent(const tcftrk::TcfTrkEvent &);
    void slotSerialPong(const QString &);

private:
    void printTimeStamp();
    bool parseArgument(const QString &a, int argNumber, QString *errorMessage);
    void handleCreateProcess(const tcftrk::TcfTrkCommandResult &result);
    void handleFileSystemOpen(const tcftrk::TcfTrkCommandResult &result);
    void handleFileSystemWrite(const tcftrk::TcfTrkCommandResult &result);
    void handleFileSystemClose(const tcftrk::TcfTrkCommandResult &result);
    void handleFileSystemFStat(const tcftrk::TcfTrkCommandResult &result);
    void handleSymbianInstall(const tcftrk::TcfTrkCommandResult &result);
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
    QScopedPointer<tcftrk::TcfTrkDevice> m_trkDevice;
};

#endif // CODACLIENTAPPLICATION_H
