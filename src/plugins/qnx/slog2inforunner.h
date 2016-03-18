/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com), KDAB (info@kdab.com)
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

#include <QObject>

#include <remotelinux/linuxdevice.h>
#include <utils/outputformat.h>

#include <QDateTime>
#include <QByteArray>

namespace ProjectExplorer { class SshDeviceProcess; }

namespace Qnx {
namespace Internal {

class Slog2InfoRunner : public QObject
{
    Q_OBJECT
public:
    explicit Slog2InfoRunner(const QString &applicationId, const RemoteLinux::LinuxDevice::ConstPtr &device, QObject *parent = 0);

    void stop();

    bool commandFound() const;

public slots:
    void start();

signals:
    void commandMissing();
    void started();
    void finished();
    void output(const QString &msg, Utils::OutputFormat format);

private slots:
    void handleTestProcessCompleted();
    void launchSlog2Info();

    void readLogStandardOutput();
    void readLogStandardError();
    void handleLogError();

private:
    void readLaunchTime();
    void processLog(bool force);
    void processLogLine(const QString &line);

    QString m_applicationId;

    bool m_found;

    QDateTime m_launchDateTime;
    bool m_currentLogs;
    QString m_remainingData;

    ProjectExplorer::SshDeviceProcess *m_launchDateTimeProcess;
    ProjectExplorer::SshDeviceProcess *m_testProcess;
    ProjectExplorer::SshDeviceProcess *m_logProcess;
};

} // namespace Internal
} // namespace Qnx
