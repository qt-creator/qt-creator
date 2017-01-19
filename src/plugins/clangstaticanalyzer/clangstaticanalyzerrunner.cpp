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

#include "clangstaticanalyzerrunner.h"

#include "clangstaticanalyzerconstants.h"

#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>
#include <utils/temporaryfile.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangstaticanalyzer.runner")

namespace ClangStaticAnalyzer {
namespace Internal {

static QString generalProcessError()
{
    return ClangStaticAnalyzerRunner::tr("An error occurred with the Clang Static Analyzer process.");
}

static QString finishedDueToCrash()
{
    return ClangStaticAnalyzerRunner::tr("Clang Static Analyzer crashed.");
}

static QStringList constructCommandLineArguments(const QString &filePath,
                                                 const QString &logFile,
                                                 const QStringList &options)
{
    QStringList arguments;

    if (LOG().isDebugEnabled())
        arguments << QLatin1String("-v");

    arguments
        << QLatin1String("--analyze")
        << QLatin1String("-o")
        << QDir::toNativeSeparators(logFile)
        ;
    arguments += options;
    arguments << QDir::toNativeSeparators(filePath);
    return arguments;
}

QString finishedWithBadExitCode(int exitCode)
{
    return ClangStaticAnalyzerRunner::tr("Clang Static Analyzer finished with exit code: %1.").arg(exitCode);
}

ClangStaticAnalyzerRunner::ClangStaticAnalyzerRunner(const QString &clangExecutable,
                                                     const QString &clangLogFileDir,
                                                     const Utils::Environment &environment,
                                                     QObject *parent)
    : QObject(parent)
    , m_clangExecutable(QDir::toNativeSeparators(clangExecutable))
    , m_clangLogFileDir(clangLogFileDir)
{
    QTC_CHECK(!m_clangExecutable.isEmpty());
    QTC_CHECK(!m_clangLogFileDir.isEmpty());

    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_process.setProcessEnvironment(environment.toProcessEnvironment());
    m_process.setWorkingDirectory(m_clangLogFileDir); // Current clang-cl puts log file into working dir.
    connect(&m_process, &QProcess::started, this, &ClangStaticAnalyzerRunner::onProcessStarted);
    connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &ClangStaticAnalyzerRunner::onProcessFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &ClangStaticAnalyzerRunner::onProcessError);
    connect(&m_process, &QProcess::readyRead, this, &ClangStaticAnalyzerRunner::onProcessOutput);
}

ClangStaticAnalyzerRunner::~ClangStaticAnalyzerRunner()
{
    Utils::SynchronousProcess::stopProcess(m_process);
}

bool ClangStaticAnalyzerRunner::run(const QString &filePath, const QStringList &compilerOptions)
{
    QTC_ASSERT(!m_clangExecutable.isEmpty(), return false);
    QTC_CHECK(!compilerOptions.contains(QLatin1String("-o")));
    QTC_CHECK(!compilerOptions.contains(filePath));

    m_filePath = filePath;
    m_processOutput.clear();

    m_logFile = createLogFile(filePath);
    QTC_ASSERT(!m_logFile.isEmpty(), return false);
    const QStringList arguments = constructCommandLineArguments(filePath, m_logFile,
                                                                compilerOptions);
    m_commandLine = Utils::QtcProcess::joinArgs(QStringList(m_clangExecutable) + arguments);

    qCDebug(LOG).noquote() << "Starting" << m_commandLine;
    m_process.start(m_clangExecutable, arguments);
    return true;
}

QString ClangStaticAnalyzerRunner::filePath() const
{
    return m_filePath;
}

void ClangStaticAnalyzerRunner::onProcessStarted()
{
    emit started();
}

void ClangStaticAnalyzerRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit) {
        if (exitCode == 0) {
            qCDebug(LOG).noquote() << "Output:\n" << Utils::SynchronousProcess::normalizeNewlines(
                                                        QString::fromLocal8Bit(m_processOutput));
            emit finishedWithSuccess(actualLogFile());
        }
        else
            emit finishedWithFailure(finishedWithBadExitCode(exitCode), processCommandlineAndOutput());
    } else { // == QProcess::CrashExit
        emit finishedWithFailure(finishedDueToCrash(), processCommandlineAndOutput());
    }
}

void ClangStaticAnalyzerRunner::onProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::Crashed)
        return; // handled by slot of finished()

    emit finishedWithFailure(generalProcessError(), processCommandlineAndOutput());
}

void ClangStaticAnalyzerRunner::onProcessOutput()
{
    m_processOutput.append(m_process.readAll());
}

QString ClangStaticAnalyzerRunner::createLogFile(const QString &filePath) const
{
    const QString fileName = QFileInfo(filePath).fileName();
    const QString fileTemplate = m_clangLogFileDir
            + QLatin1String("/report-") + fileName + QLatin1String("-XXXXXX.plist");

    Utils::TemporaryFile temporaryFile("clangstaticanalyzer");
    temporaryFile.setAutoRemove(false);
    temporaryFile.setFileTemplate(fileTemplate);
    if (temporaryFile.open()) {
        temporaryFile.close();
        return temporaryFile.fileName();
    }
    return QString();
}

QString ClangStaticAnalyzerRunner::processCommandlineAndOutput() const
{
    return tr("Command line: %1\n"
                       "Process Error: %2\n"
                       "Output:\n%3")
                            .arg(m_commandLine,
                                 QString::number(m_process.error()),
                                 Utils::SynchronousProcess::normalizeNewlines(
                                     QString::fromLocal8Bit(m_processOutput)));
}

QString ClangStaticAnalyzerRunner::actualLogFile() const
{
    if (QFileInfo(m_logFile).size() == 0) {
        // Current clang-cl ignores -o, always putting the log file into the working directory.
        return m_clangLogFileDir + QLatin1Char('/') + QFileInfo(m_filePath).completeBaseName()
                + QLatin1String(".plist");
    }
    return m_logFile;
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
