/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion and Hugues Delorme
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

#ifndef VCSBASE_COMMAND_H
#define VCSBASE_COMMAND_H

#include "vcsbase_global.h"

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QProcessEnvironment>

namespace VcsBase {

namespace Internal { class CommandPrivate; }

class VCSBASE_EXPORT Command : public QObject
{
    Q_OBJECT

public:
    // Where to report command termination with exit code if desired
    enum TerminationReportMode
    {
        NoReport,
        ReportStdout,  // This assumes UTF8
        ReportStderr
    };

    Command(const QString &binary,
            const QString &workingDirectory,
            const QProcessEnvironment &environment);
    ~Command();

    void addJob(const QStringList &arguments);
    void addJob(const QStringList &arguments, int timeout);
    void execute();
    bool lastExecutionSuccess() const;
    int lastExecutionExitCode() const;

    // Clean output from carriage return and ANSI color codes
    // Workaround until all relevant commands support "--no-color"
    static void removeColorCodes(QByteArray *data);

    const QString &binaryPath() const;
    const QString &workingDirectory() const;
    const QProcessEnvironment &processEnvironment() const;

    // Report command termination with exit code
    TerminationReportMode reportTerminationMode() const;
    void setTerminationReportMode(TerminationReportMode m);

    int defaultTimeout() const;
    void setDefaultTimeout(int timeout);

    // Disable Terminal on UNIX (see VCS SSH handling)
    bool unixTerminalDisabled() const;
    void setUnixTerminalDisabled(bool);

    static QString msgTimeout(int seconds);

    const QVariant &cookie() const;
    void setCookie(const QVariant &cookie);

private:
    void run();

signals:
    void outputData(const QByteArray &);
    void errorText(const QString &);
    void finished(bool ok, int exitCode, const QVariant &cookie);
    void success(const QVariant &cookie);

private:
    class Internal::CommandPrivate *const d;
};

} // namespace VcsBase

#endif // VCSBASE_COMMAND_H
