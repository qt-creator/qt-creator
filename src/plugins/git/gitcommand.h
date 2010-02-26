/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef GITCOMMAND_H
#define GITCOMMAND_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

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

    explicit GitCommand(const QStringList &binary,
                        const QString &workingDirectory,
                        const QStringList &environment,
                        const QVariant &cookie = QVariant());


    void addJob(const QStringList &arguments, int timeout);
    void execute();

    // Clean output from carriage return and ANSI color codes.
    // Workaround until all relevant commands support "--no-color".
    static void removeColorCodes(QByteArray *data);

    // Report command termination with exit code
    TerminationReportMode reportTerminationMode() const;
    void setTerminationReportMode(TerminationReportMode m);

    static QString msgTimeout(int seconds);
    // Helper to kill a process by SIGNAL first, allowing for cleanup
    static bool stopProcess(QProcess &p);

private:
    void run();

Q_SIGNALS:
    void outputData(const QByteArray&);
    void errorText(const QString&);
    void finished(bool ok, const QVariant &cookie);
    void success();

private:
    struct Job {
        explicit Job(const QStringList &a, int t);

        QStringList arguments;
        int timeout;
    };

    const QString m_binaryPath;
    QStringList m_basicArguments;
    const QString m_workingDirectory;
    const QStringList m_environment;
    const QVariant m_cookie;

    QList<Job> m_jobs;
    TerminationReportMode m_reportTerminationMode;
};

} // namespace Internal
} // namespace Git
#endif // GITCOMMAND_H
