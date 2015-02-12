/**************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_SLOG2INFORUNNER_H
#define QNX_INTERNAL_SLOG2INFORUNNER_H

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

#endif // QNX_INTERNAL_SLOG2INFORUNNER_H
