/**************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef BLACKBERRYSLOGPROCESS_H
#define BLACKBERRYSLOGPROCESS_H

#include "blackberrydeviceconfiguration.h"

#include <utils/outputformat.h>
#include <ssh/sshconnection.h>

#include <QDateTime>

#include <QObject>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace Qnx {
namespace Internal {

class BlackBerryLogProcessRunner : public QObject
{
    Q_OBJECT
public:
    explicit BlackBerryLogProcessRunner(QObject *parent, const QString &appId, const BlackBerryDeviceConfiguration::ConstPtr &device);
    void start();

signals:
    void output(const QString &msg, Utils::OutputFormat format);
    void finished();

public slots:
    void stop();

private slots:
    void handleSlog2infoFound();
    void readLaunchTime();
    void readApplicationLog();

    void handleTailOutput();
    void handleTailProcessConnectionError();

    void handleSlog2infoOutput();
    void handleSlog2infoProcessConnectionError();

    void handleLogError();

private:
    QString m_appId;
    QString m_tailCommand;
    QString m_slog2infoCommand;

    bool m_currentLogs;
    bool m_slog2infoFound;

    QDateTime m_launchDateTime;

    BlackBerryDeviceConfiguration::ConstPtr m_device;
    QSsh::SshConnectionParameters m_sshParams;

    QSsh::SshRemoteProcessRunner *m_tailProcess;
    QSsh::SshRemoteProcessRunner *m_slog2infoProcess;

    QSsh::SshRemoteProcessRunner *m_testSlog2Process;
    QSsh::SshRemoteProcessRunner *m_launchDateTimeProcess;

    bool showQtMessage(const QString& pattern, const QString& line);
    void killProcessRunner(QSsh::SshRemoteProcessRunner *processRunner, const QString& command);
};
} // namespace Internal
} // namespace Qnx

#endif // BLACKBERRYSLOGPROCESS_H
