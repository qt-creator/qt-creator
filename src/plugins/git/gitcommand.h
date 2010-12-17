/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GITCOMMAND_H
#define GITCOMMAND_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QProcessEnvironment>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

// Asynchronous command with output signals and a finished
// signal with a magic cookie
class GitCommand : public QObject
{
    Q_DISABLE_COPY(GitCommand)
    Q_OBJECT
public:
    // Where to report command termination with exit code if desired
    enum TerminationReportMode { NoReport,
                                 ReportStdout,  // This assumes UTF8
                                 ReportStderr };

    GitCommand(const QString &binary,
               const QString &workingDirectory,
               const QProcessEnvironment &environment,
               const QVariant &cookie = QVariant());

    void addJob(const QStringList &arguments, int timeout);
    void execute();

    // Clean output from carriage return and ANSI color codes.
    // Workaround until all relevant commands support "--no-color".
    static void removeColorCodes(QByteArray *data);

    // Report command termination with exit code
    TerminationReportMode reportTerminationMode() const;
    void setTerminationReportMode(TerminationReportMode m);

    // Disable Terminal on UNIX (see VCS SSH handling).
    bool unixTerminalDisabled() const;
    void setUnixTerminalDisabled(bool);

    static QString msgTimeout(int seconds);

    void setCookie(const QVariant &cookie);
    QVariant cookie() const;

private:
    void run();

Q_SIGNALS:
    void outputData(const QByteArray&);
    void errorText(const QString&);
    void finished(bool ok, int exitCode, const QVariant &cookie);
    void success();

private:
    struct Job {
        explicit Job(const QStringList &a, int t);

        QStringList arguments;
        int timeout;
    };

    const QString m_binaryPath;
    const QString m_workingDirectory;
    const QProcessEnvironment m_environment;
    QVariant m_cookie;
    bool m_unixTerminalDisabled;

    QList<Job> m_jobs;
    TerminationReportMode m_reportTerminationMode;
};

} // namespace Internal
} // namespace Git
#endif // GITCOMMAND_H
