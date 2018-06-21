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

#include <projectexplorer/runconfiguration.h>
#include <remotelinux/linuxdevice.h>
#include <utils/outputformat.h>

#include <QDateTime>
#include <QByteArray>

namespace ProjectExplorer { class SshDeviceProcess; }

namespace Qnx {
namespace Internal {

class Slog2InfoRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT
public:
    explicit Slog2InfoRunner(ProjectExplorer::RunControl *runControl);

    void start() override;
    void stop() override;

    bool commandFound() const;

signals:
    void commandMissing();
    void started();
    void finished();

private:
    void handleTestProcessCompleted();
    void launchSlog2Info();

    void readLogStandardOutput();
    void readLogStandardError();
    void handleLogError();

    void printMissingWarning();
    void readLaunchTime();
    void processLog(bool force);
    void processLogLine(const QString &line);

    QString m_applicationId;

    QDateTime m_launchDateTime;
    bool m_found = false;
    bool m_currentLogs = false;
    QString m_remainingData;

    ProjectExplorer::SshDeviceProcess *m_launchDateTimeProcess = nullptr;
    ProjectExplorer::SshDeviceProcess *m_testProcess = nullptr;
    ProjectExplorer::SshDeviceProcess *m_logProcess = nullptr;
};

} // namespace Internal
} // namespace Qnx
