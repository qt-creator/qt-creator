/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef IOSRUNNER_H
#define IOSRUNNER_H

#include "iosconfigurations.h"
#include "iostoolhandler.h"
#include "iossimulator.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QProcess>
#include <QMutex>

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
    IosDeviceType m_deviceType;
    bool m_cppDebug;
    bool m_qmlDebug;
    bool m_cleanExit;
    quint16 m_qmlPort;
    Q_PID m_pid;
};

} // namespace Internal
} // namespace Ios

#endif // IOSRUNNER_H
