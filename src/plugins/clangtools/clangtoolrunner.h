/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QString>
#include <QProcess>

namespace Utils { class Environment; }

namespace ClangTools {
namespace Internal {

QString finishedWithBadExitCode(const QString &name, int exitCode); // exposed for tests

class ClangToolRunner : public QObject
{
    Q_OBJECT

public:
    ClangToolRunner(const QString &clangExecutable,
                    const QString &clangLogFileDir,
                    const Utils::Environment &environment,
                    const QString &name,
                    QObject *parent = nullptr);
    ~ClangToolRunner() override;

    // compilerOptions is expected to contain everything except:
    //   (1) filePath, that is the file to analyze
    //   (2) -o output-file
    bool run(const QString &filePath, const QStringList &compilerOptions = QStringList());

    QString filePath() const { return m_filePath; }
    QString logFilePath() const { return m_logFile; }

signals:
    void started();
    void finishedWithSuccess(const QString &filePath);
    void finishedWithFailure(const QString &errorMessage, const QString &errorDetails);

protected:
    virtual QStringList constructCommandLineArguments(const QStringList &options) = 0;

    virtual void onProcessOutput();

private:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

    QString createLogFile(const QString &filePath) const;
    QString processCommandlineAndOutput() const;

protected:
    QString m_logFile;
    QProcess m_process;
    QByteArray m_processOutput;

private:
    QString m_clangExecutable;
    QString m_clangLogFileDir;
    QString m_filePath;
    QString m_commandLine;
    const QString m_name;
};

} // namespace Internal
} // namespace ClangTools
