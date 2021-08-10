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
#include <utils/temporaryfile.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.runner", QtWarningMsg)

using namespace Utils;

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

ClangToolRunner::ClangToolRunner(QObject *parent)
    : QObject(parent), m_process(new Utils::QtcProcess)
{}

ClangToolRunner::~ClangToolRunner()
{
    if (m_process->state() != QProcess::NotRunning) {
        // asking politly to terminate costs ~300 ms on windows so skip the courtasy and direct kill the process
        if (Utils::HostOsInfo::isWindowsHost()) {
            m_process->kill();
            m_process->waitForFinished(100);
        } else {
            m_process->stopProcess();
        }
    }

    m_process->deleteLater();
}

void ClangToolRunner::init(const FilePath &outputDirPath, const Environment &environment)
{
    m_outputDirPath = outputDirPath;
    QTC_CHECK(!m_outputDirPath.isEmpty());

    m_process->setEnvironment(environment);
    m_process->setWorkingDirectory(m_outputDirPath); // Current clang-cl puts log file into working dir.
    connect(m_process, &QtcProcess::finished, this, &ClangToolRunner::onProcessFinished);
    connect(m_process, &QtcProcess::errorOccurred, this, &ClangToolRunner::onProcessError);
}

QStringList ClangToolRunner::mainToolArguments() const
{
    QStringList result;
    result << "-export-fixes=" + m_outputFilePath;
    if (!m_overlayFilePath.isEmpty() && supportsVFSOverlay())
        result << "--vfsoverlay=" + m_overlayFilePath;
    result << QDir::toNativeSeparators(m_fileToAnalyze);
    return result;
}

bool ClangToolRunner::supportsVFSOverlay() const
{
    static QMap<QString, bool> vfsCapabilities;
    auto it = vfsCapabilities.find(m_executable);
    if (it == vfsCapabilities.end()) {
        QtcProcess p;
        p.setCommand({FilePath::fromString(m_executable), {"--help"}});
        p.runBlocking();
        it = vfsCapabilities.insert(m_executable, p.allOutput().contains("vfsoverlay"));
    }
    return it.value();
}

static QString createOutputFilePath(const FilePath &dirPath, const QString &fileToAnalyze)
{
    const QString fileName = QFileInfo(fileToAnalyze).fileName();
    const FilePath fileTemplate = dirPath.pathAppended("report-" + fileName + "-XXXXXX");

    TemporaryFile temporaryFile("clangtools");
    temporaryFile.setAutoRemove(false);
    temporaryFile.setFileTemplate(fileTemplate.path());
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

    m_outputFilePath = createOutputFilePath(m_outputDirPath, fileToAnalyze);
    QTC_ASSERT(!m_outputFilePath.isEmpty(), return false);
    m_commandLine = {FilePath::fromString(m_executable), m_argsCreator(compilerOptions)};

    qCDebug(LOG).noquote() << "Starting" << m_commandLine.toUserOutput();
    m_process->setCommand(m_commandLine);
    m_process->start();
    return true;
}

void ClangToolRunner::onProcessFinished()
{
    if (m_process->result() == QtcProcess::FinishedWithSuccess) {
        qCDebug(LOG).noquote() << "Output:\n" << m_process->stdOut();
        emit finishedWithSuccess(m_fileToAnalyze);
    } else if (m_process->result() == QtcProcess::FinishedWithError) {
        emit finishedWithFailure(finishedWithBadExitCode(m_name, m_process->exitCode()),
                                 commandlineAndOutput());
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

QString ClangToolRunner::commandlineAndOutput() const
{
    return tr("Command line: %1\n"
              "Process Error: %2\n"
              "Output:\n%3")
        .arg(m_commandLine.toUserOutput())
        .arg(m_process->error())
        .arg(m_process->stdOut());
}

} // namespace Internal
} // namespace ClangTools
