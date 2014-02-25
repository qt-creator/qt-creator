/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#ifndef IOSRUNNER_H
#define IOSRUNNER_H

#include "iosconfigurations.h"

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QProcess>
#include <QMutex>
#include "iostoolhandler.h"
#include <projectexplorer/devicesupport/idevice.h>

namespace Ios {
namespace Internal {

class IosRunConfiguration;

class IosRunner : public QObject
{
    Q_OBJECT

public:
    IosRunner(QObject *parent, IosRunConfiguration *runConfig, bool cppDebug, bool qmlDebug);
    ~IosRunner();

    QString displayName() const;

    QString bundlePath();
    QStringList extraArgs();
    QString deviceId();
    IosToolHandler::RunKind runType();
    bool cppDebug() const;
    bool qmlDebug() const;
public slots:
    void start();
    void stop();

signals:
    void didStartApp(Ios::IosToolHandler::OpStatus status);
    void gotServerPorts(int gdbPort, int qmlPort);
    void gotInferiorPid(Q_PID pid, int);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
    void finished(bool cleanExit);
private slots:
    void handleDidStartApp(Ios::IosToolHandler *handler, const QString &bundlePath,
                           const QString &deviceId, Ios::IosToolHandler::OpStatus status);
    void handleGotServerPorts(Ios::IosToolHandler *handler, const QString &bundlePath,
                              const QString &deviceId, int gdbPort, int qmlPort);
    void handleGotInferiorPid(Ios::IosToolHandler *handler, const QString &bundlePath,
                              const QString &deviceId, Q_PID pid);
    void handleAppOutput(Ios::IosToolHandler *handler, const QString &output);
    void handleErrorMsg(Ios::IosToolHandler *handler, const QString &msg);
    void handleToolExited(Ios::IosToolHandler *handler, int code);
    void handleFinished(Ios::IosToolHandler *handler);
private:
    IosToolHandler *m_toolHandler;
    QString m_bundleDir;
    QStringList m_arguments;
    ProjectExplorer::IDevice::ConstPtr m_device;
    bool m_cppDebug;
    bool m_qmlDebug;
    bool m_cleanExit;
    quint16 m_qmlPort;
    Q_PID m_pid;
};

} // namespace Internal
} // namespace Ios

#endif // IOSRUNNER_H
