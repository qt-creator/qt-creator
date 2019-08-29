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

#include "clangtoolrunner.h"

#include "clangtoolsconstants.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>
#include <utils/temporaryfile.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.runner", QtWarningMsg)

namespace ClangTools {
namespace Internal {

static QString generalProcessError(const QString &name)
{
    return ClangToolRunner::tr("An error occurred with the %1 process.").arg(name);
}

static QString finishedDueToCrash(const QString &name)
{
    return ClangToolRunner::tr("%1 crashed.").arg(name);
}

static QString finishedWithBadExitCode(const QString &name, int exitCode)
{
    return ClangToolRunner::tr("%1 finished with exit code: %2.").arg(name).arg(exitCode);
}

void ClangToolRunner::init(const QString &outputDirPath,
                           const Utils::Environment &environment)
{
    m_outputDirPath = outputDirPath;
    QTC_CHECK(!m_outputDirPath.isEmpty());

    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_process.setProcessEnvironment(environment.toProcessEnvironment());
    m_process.setWorkingDirectory(m_outputDirPath); // Current clang-cl puts log file into working dir.
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ClangToolRunner::onProcessFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &ClangToolRunner::onProcessError);
    connect(&m_process, &QProcess::readyRead, this, &ClangToolRunner::onProcessOutput);
}

ClangToolRunner::~ClangToolRunner()
{
    Utils::SynchronousProcess::stopProcess(m_process);
}

static QString createOutputFilePath(const QString &dirPath, const QString &fileToAnalyze)
{
    const QString fileName = QFileInfo(fileToAnalyze).fileName();
    const QString fileTemplate = dirPath
            + QLatin1String("/report-") + fileName + QLatin1String("-XXXXXX");

    Utils::TemporaryFile temporaryFile("clangtools");
    temporaryFile.setAutoRemove(false);
    temporaryFile.setFileTemplate(fileTemplate);
    if (temporaryFile.open()) {
        temporaryFile.close();
        return temporaryFile.fileName();
    }
    return QString();
}

bool ClangToolRunner::run(const QString &fileToAnalyze, const QStringList &compilerOptions)
{
    QTC_ASSERT(!m_executable.isEmpty(), return false);
    QTC_CHECK(!compilerOptions.contains(QLatin1String("-o")));
    QTC_CHECK(!compilerOptions.contains(fileToAnalyze));

    m_fileToAnalyze = fileToAnalyze;
    m_processOutput.clear();

    m_outputFilePath = createOutputFilePath(m_outputDirPath, fileToAnalyze);
    QTC_ASSERT(!m_outputFilePath.isEmpty(), return false);
    const QStringList arguments = m_argsCreator(compilerOptions);
    m_commandLine = Utils::QtcProcess::joinArgs(QStringList(m_executable) + arguments);

    qCDebug(LOG).noquote() << "Starting" << m_commandLine;
    m_process.start(m_executable, arguments);
    return true;
}

void ClangToolRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit) {
        if (exitCode == 0) {
            qCDebug(LOG).noquote() << "Output:\n" << Utils::SynchronousProcess::normalizeNewlines(
                                                        QString::fromLocal8Bit(m_processOutput));
            emit finishedWithSuccess(m_fileToAnalyze);
        } else {
            emit finishedWithFailure(finishedWithBadExitCode(m_name, exitCode),
                                     commandlineAndOutput());
        }
    } else { // == QProcess::CrashExit
        emit finishedWithFailure(finishedDueToCrash(m_name), commandlineAndOutput());
    }
}

void ClangToolRunner::onProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::Crashed)
        return; // handled by slot of finished()

    emit finishedWithFailure(generalProcessError(m_name), commandlineAndOutput());
}

void ClangToolRunner::onProcessOutput()
{
    m_processOutput.append(m_process.readAll());
}

QString ClangToolRunner::commandlineAndOutput() const
{
    return tr("Command line: %1\n"
              "Process Error: %2\n"
              "Output:\n%3")
        .arg(m_commandLine,
             QString::number(m_process.error()),
             Utils::SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(m_processOutput)));
}

} // namespace Internal
} // namespace ClangTools
