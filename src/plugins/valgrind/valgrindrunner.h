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

#include <debugger/analyzer/analyzerconstants.h>

#include <projectexplorer/runnables.h>

#include <utils/outputformat.h>
#include <ssh/sshconnection.h>

#include <QProcess>

namespace Valgrind {

class ValgrindProcess;

class ValgrindRunner : public QObject
{
    Q_OBJECT

public:
    explicit ValgrindRunner(QObject *parent = 0);
    ~ValgrindRunner();

    QString valgrindExecutable() const;
    void setValgrindExecutable(const QString &executable);
    QStringList valgrindArguments() const;
    QStringList fullValgrindArguments() const;
    void setValgrindArguments(const QStringList &toolArguments);
    void setDebuggee(const ProjectExplorer::StandardRunnable &debuggee) ;
    void setProcessChannelMode(QProcess::ProcessChannelMode mode);

    void setDevice(const ProjectExplorer::IDevice::ConstPtr &device);
    ProjectExplorer::IDevice::ConstPtr device() const;

    void waitForFinished() const;

    QString errorString() const;

    virtual bool start();
    virtual void stop();

    ValgrindProcess *valgrindProcess() const;

signals:
    void processOutputReceived(const QString &, Utils::OutputFormat);
    void processErrorReceived(const QString &, QProcess::ProcessError);
    void started();
    void finished();

protected:
    virtual QString tool() const = 0;
    virtual void processError(QProcess::ProcessError);
    virtual void processFinished(int, QProcess::ExitStatus);
    virtual void localHostAddressRetrieved(const QHostAddress &localHostAddress);

private:
    class Private;
    Private *d;
};

} // namespace Valgrind
