/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <projectexplorer/devicesupport/idevice.h>

#include <QObject>
#include <QProcess>
#include <QTimer>

namespace Android {
namespace Internal {

class AndroidSignalOperation : public ProjectExplorer::DeviceProcessSignalOperation
{
    Q_OBJECT
public:
    ~AndroidSignalOperation() {}
    void killProcess(qint64 pid);
    void killProcess(const QString &filePath);
    void interruptProcess(qint64 pid);
    void interruptProcess(const QString &filePath);

protected:
    explicit AndroidSignalOperation();

private:
    void adbFindRunAsFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void adbKillFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleTimeout();

    void signalOperationViaADB(qint64 pid, int signal);

    QString m_adbPath;
    QProcess *m_adbProcess;
    QTimer *m_timeout;

    enum State {
        Idle,
        RunAs,
        Kill
    } m_state;

    qint64 m_pid;
    int m_signal;

    friend class AndroidDevice;
};

} // namespace Internal
} // namespace Android
