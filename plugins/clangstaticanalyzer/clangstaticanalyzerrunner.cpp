/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerrunner.h"

#include "clangstaticanalyzerconstants.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QTemporaryFile>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangstaticanalyzer.runner")

static QString generalProcessError()
{
    return QObject::tr("An error occurred with the clang static analyzer process.");
}

static QString finishedDueToCrash()
{
    return QObject::tr("Clang static analyzer crashed.");
}

static QStringList constructCommandLineArguments(const QString &filePath,
                                                 const QString &logFile,
                                                 const QStringList &definesAndIncludes)
{
    QStringList arguments = QStringList()
        << QLatin1String("--analyze")
        << QLatin1String("-o")
        << logFile
        ;
    arguments += definesAndIncludes;
    arguments << filePath;
    return arguments;
}

namespace ClangStaticAnalyzer {
namespace Internal {

QString finishedWithBadExitCode(int exitCode)
{
    return QObject::tr("Clang static analyzer finished with exit code: %1.").arg(exitCode);
}

ClangStaticAnalyzerRunner::ClangStaticAnalyzerRunner(const QString &clangExecutable,
                                                     const QString &clangLogFileDir,
                                                     QObject *parent)
    : QObject(parent)
    , m_clangExecutable(clangExecutable)
    , m_clangLogFileDir(clangLogFileDir)
{
    QTC_CHECK(!m_clangExecutable.isEmpty());
    QTC_CHECK(!m_clangLogFileDir.isEmpty());

    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::started,
            this, &ClangStaticAnalyzerRunner::onProcessStarted);
    connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &ClangStaticAnalyzerRunner::onProcessFinished);
    connect(&m_process, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, &ClangStaticAnalyzerRunner::onProcessError);
    connect(&m_process, &QProcess::readyRead,
            this, &ClangStaticAnalyzerRunner::onProcessOutput);
}

ClangStaticAnalyzerRunner::~ClangStaticAnalyzerRunner()
{
    const QProcess::ProcessState processState = m_process.state();
    if (processState == QProcess::Starting || processState == QProcess::Running)
        m_process.kill();
}

bool ClangStaticAnalyzerRunner::run(const QString &filePath, const QStringList &definesAndIncludes)
{
    QTC_ASSERT(!m_clangExecutable.isEmpty(), return false);

    m_processOutput.clear();

    m_logFile = createLogFile(filePath);
    QTC_ASSERT(!m_logFile.isEmpty(), return false);
    const QStringList arguments = constructCommandLineArguments(filePath, m_logFile,
                                                                definesAndIncludes);
    m_commandLine = m_clangExecutable + QLatin1Char(' ') + arguments.join(QLatin1Char(' '));

    qCDebug(LOG) << "Starting" << m_commandLine;
    m_process.start(m_clangExecutable, arguments);
    return true;
}

void ClangStaticAnalyzerRunner::onProcessStarted()
{
    emit started();
}

void ClangStaticAnalyzerRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit) {
        if (exitCode == 0)
            emit finishedWithSuccess(m_logFile);
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

    QTemporaryFile temporaryFile;
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
    return QObject::tr("Command line: \"%1\"\n"
                       "Process Error: \"%2\"\n"
                       "Output:\n\"%3\"")
                            .arg(m_commandLine,
                                 QString::number(m_process.error()),
                                 QString::fromLocal8Bit(m_processOutput));
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
