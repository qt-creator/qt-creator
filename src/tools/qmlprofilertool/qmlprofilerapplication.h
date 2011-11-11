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

#ifndef QMLPROFILERAPPLICATION_H
#define QMLPROFILERAPPLICATION_H

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include <qdeclarativedebugclient.h>
#include <qmlprofilertraceclient.h>
#include <qmlprofilereventlist.h>
#include <qv8profilerclient.h>

QT_FORWARD_DECLARE_CLASS(QProcess)

class QmlProfilerApplication : public QCoreApplication
{
    Q_OBJECT
public:
    QmlProfilerApplication(int &argc, char **argv);
    ~QmlProfilerApplication();

    bool parseArguments();
    void printUsage();
    int exec();

signals:
    void done();

public slots:
    void userCommand(const QString &command);

private slots:
    void run();
    void tryToConnect();
    void connected();
    void connectionStateChanged(QAbstractSocket::SocketState state);
    void connectionError(QAbstractSocket::SocketError error);
    void processHasOutput();
    void processFinished();

    void traceClientEnabled();
    void traceFinished();
    void recordingChanged();

    void print(const QString &line);
    void logError(const QString &error);
    void logStatus(const QString &status);

    void qmlComplete();
    void v8Complete();

private:
    void printCommands();
    QString traceFileName() const;

    enum ApplicationMode {
        LaunchMode,
        AttachMode
    } m_runMode;

    // LaunchMode
    QString m_programPath;
    QStringList m_programArguments;
    QProcess *m_process;
    QString m_tracePrefix;

    QString m_hostName;
    quint16 m_port;
    bool m_verbose;
    bool m_quitAfterSave;

    QmlJsDebugClient::QDeclarativeDebugConnection m_connection;
    QmlJsDebugClient::QmlProfilerTraceClient m_traceClient;
    QmlJsDebugClient::QV8ProfilerClient m_v8profilerClient;
    QmlJsDebugClient::QmlProfilerEventList m_eventList;
    QTimer m_connectTimer;
    uint m_connectionAttempts;

    bool m_qmlDataReady;
    bool m_v8DataReady;
};

#endif // QMLPROFILERAPPLICATION_H
