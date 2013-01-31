/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDRUNNER_H
#define ANDROIDRUNNER_H

#include "androidconfigurations.h"

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QProcess>
#include <QMutex>

namespace Android {
namespace Internal {

class AndroidRunConfiguration;

class AndroidRunner : public QThread
{
    Q_OBJECT

public:
    AndroidRunner(QObject *parent, AndroidRunConfiguration *runConfig, bool debuggingMode);
    ~AndroidRunner();

    QString displayName() const;

public slots:
    void start();
    void stop();

signals:
    void remoteProcessStarted(int gdbServerPort = -1, int qmlPort = -1);
    void remoteProcessFinished(const QString &errString = QString());

    void remoteOutput(const QByteArray &output);
    void remoteErrorOutput(const QByteArray &output);

private slots:
    void killPID();
    void checkPID();
    void logcatReadStandardError();
    void logcatReadStandardOutput();
    void startLogcat();
    void asyncStart();

private:
    void adbKill(qint64 pid, const QString &device, int timeout = 2000, const QString &runAsPackageName = QString());

private:
    QProcess m_adbLogcatProcess;
    QByteArray m_logcat;
    QString m_intentName;
    QString m_packageName;
    QString m_deviceSerialNumber;
    qint64 m_processPID;
    qint64 m_gdbserverPID;
    QTimer m_checkPIDTimer;
    bool m_useCppDebugger;
    bool m_useQmlDebugger;
    QString m_remoteGdbChannel;
    uint m_qmlPort;
    bool m_useLocalQtLibs;
    QString m_localLibs;
    QString m_localJars;
    QString m_localJarsInitClasses;
    QMutex m_mutex;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDRUNNER_H
