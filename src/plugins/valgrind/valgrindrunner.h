/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include <projectexplorer/runconfiguration.h>

#include <utils/outputformat.h>

#include <QProcess>

namespace Valgrind {

namespace XmlProtocol { class ThreadedParser; }

class ValgrindRunner : public QObject
{
    Q_OBJECT

public:
    explicit ValgrindRunner(QObject *parent = nullptr);
    ~ValgrindRunner() override;

    void setValgrindExecutable(const QString &executable);
    void setValgrindArguments(const QStringList &toolArguments);
    void setDebuggee(const ProjectExplorer::Runnable &debuggee);
    void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    void setLocalServerAddress(const QHostAddress &localServerAddress);
    void setDevice(const ProjectExplorer::IDevice::ConstPtr &device);
    void setUseTerminal(bool on);

    void waitForFinished() const;

    QString errorString() const;

    bool start();
    void stop();

    XmlProtocol::ThreadedParser *parser() const;

signals:
    void logMessageReceived(const QByteArray &);
    void processOutputReceived(const QString &, Utils::OutputFormat);
    void processErrorReceived(const QString &, QProcess::ProcessError);
    void valgrindExecuted(const QString &);
    void valgrindStarted(qint64 pid);
    void finished();
    void extraProcessFinished();

private:
    bool startServers();
    void processError(QProcess::ProcessError);
    void processFinished(int, QProcess::ExitStatus);

    void xmlSocketConnected();
    void logSocketConnected();
    void readLogSocket();

    class Private;
    Private *d;
};

} // namespace Valgrind
