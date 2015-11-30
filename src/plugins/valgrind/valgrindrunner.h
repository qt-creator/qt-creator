/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#ifndef VALGRIND_RUNNER_H
#define VALGRIND_RUNNER_H

#include <analyzerbase/analyzerconstants.h>
#include <projectexplorer/applicationlauncher.h>

#include <utils/outputformat.h>
#include <ssh/sshconnection.h>

#include <QProcess>

namespace Utils {
class Environment;
class SshConnectionParameters;
}

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
    QString debuggeeExecutable() const;
    void setDebuggeeExecutable(const QString &executable);
    QString debuggeeArguments() const;
    void setDebuggeeArguments(const QString &arguments);

    void setWorkingDirectory(const QString &path);
    QString workingDirectory() const;
    void setEnvironment(const Utils::Environment &environment);
    void setProcessChannelMode(QProcess::ProcessChannelMode mode);

    void setUseStartupProject(bool useStartupProject);
    bool useStartupProject() const;

    void setLocalRunMode(ProjectExplorer::ApplicationLauncher::Mode localRunMode);
    ProjectExplorer::ApplicationLauncher::Mode localRunMode() const;

    void setConnectionParameters(const QSsh::SshConnectionParameters &connParams);
    const QSsh::SshConnectionParameters &connectionParameters() const;

    void waitForFinished() const;

    QString errorString() const;

    virtual bool start();
    virtual void stop();

    ValgrindProcess *valgrindProcess() const;

protected:
    virtual QString tool() const = 0;

signals:
    void processOutputReceived(const QString &, Utils::OutputFormat);
    void processErrorReceived(const QString &, QProcess::ProcessError);
    void started();
    void finished();

protected slots:
    virtual void processError(QProcess::ProcessError);
    virtual void processFinished(int, QProcess::ExitStatus);
    virtual void localHostAddressRetrieved(const QHostAddress &localHostAddress);

private:
    class Private;
    Private *d;
};

} // namespace Valgrind

#endif // VALGRIND_RUNNER_H
