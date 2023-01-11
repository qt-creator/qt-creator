// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishrunnerprocess.h"

#include "squishtr.h"

#include <debugger/breakhandler.h>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(runnerLOG, "qtc.squish.squishrunner", QtWarningMsg)

namespace Squish::Internal {

SquishRunnerProcess::SquishRunnerProcess(QObject *parent)
    : SquishProcessBase{parent}
{
}

void SquishRunnerProcess::setupProcess(RunnerMode mode)
{
    QTC_ASSERT(!m_mode || m_mode == mode, return);
    if (m_mode == mode)
        return;

    m_mode.emplace(mode);
    switch (m_mode.value()) {
    case Run:
    case StartAut:
        m_process.setProcessMode(Utils::ProcessMode::Writer);
        m_process.setStdOutLineCallback([this](const QString &line){ onStdOutput(line); });
        break;
    case QueryServer:
        break;
    }
}

void SquishRunnerProcess::start(const Utils::CommandLine &cmdline, const Utils::Environment &env)
{
    QTC_ASSERT(m_process.state() == QProcess::NotRunning, return);
    m_licenseIssues = false;
    m_autId = 0;

    SquishProcessBase::start(cmdline, env);
}

void SquishRunnerProcess::onDone()
{
    qCDebug(runnerLOG) << "Runner finished";
    if (m_mode == QueryServer) {
        const QString error = m_licenseIssues ? Tr::tr("Could not get Squish license from server.")
                                              : QString();
        emit queryDone(m_process.stdOut(), error);
    } else {
        emit runnerFinished(); // handle output file stuff - FIXME move over to runner?
    }
    setState(Stopped);
}

void SquishRunnerProcess::onErrorOutput()
{
    // output that must be sent to the Runner/Server Log
    const QByteArray output = m_process.readAllRawStandardError();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        emit logOutputReceived("Runner: " + QLatin1String(trimmed));
        if (trimmed.startsWith("QSocketNotifier: Invalid socket")) {
            emit runnerError(InvalidSocket);
        } else if (trimmed.contains("could not be started.")
                   && trimmed.contains("Mapped AUT")) {
            emit runnerError(MappedAutMissing);
        } else if (trimmed.startsWith("Couldn't get license")
                   || trimmed.contains("UNLICENSED version of Squish")) {
            m_licenseIssues = true;
        }
    }
}

void SquishRunnerProcess::onStdOutput(const QString &lineIn)
{
    int fileLine = -1;
    int fileColumn = -1;
    QString fileName;
    // we might enter this function by invoking it directly instead of getting signaled
    bool isPrompt = false;
    QString line = lineIn;
    line.chop(1); // line has a newline
    if (line.startsWith("SDBG:"))
        line = line.mid(5);
    if (line.isEmpty()) // we have a prompt
        isPrompt = true;
    else if (line.startsWith("symb")) { // symbols information (locals)
        isPrompt = true;
        // paranoia
        if (!line.endsWith("}"))
            return;
        if (line.at(4) == '.') { // single symbol information
            line = line.mid(5);
            emit localsUpdated(line);
        } else { // line.at(4) == ':' // all locals
            line = line.mid(6);
            line.chop(1);
            emit localsUpdated(line);
        }
    } else if (line.startsWith("@line")) { // location information (interrupted)
        isPrompt = true;
        // paranoia
        if (!line.endsWith(":"))
            return;

        const QStringList locationParts = line.split(',');
        QTC_ASSERT(locationParts.size() == 3, return);
        fileLine = locationParts[0].mid(6).toInt();
        fileColumn = locationParts[1].mid(7).toInt();
        fileName = locationParts[2].trimmed();
        fileName.chop(1); // remove the colon
        const Utils::FilePath fp = Utils::FilePath::fromUserInput(fileName);
        if (fp.isRelativePath())
            fileName = m_currentTestCasePath.resolvePath(fileName).toString();
    } else if (m_autId == 0 && line.startsWith("AUTID: ")) {
        isPrompt = true;
        m_autId = line.mid(7).toInt();
        qCInfo(runnerLOG) << "AUT ID set" << m_autId << "(" << line << ")";
    }
    if (isPrompt)
        emit interrupted(fileName, fileLine, fileColumn);
}

static QString cmdToString(SquishRunnerProcess::RunnerCommand cmd)
{
    switch (cmd) {
    case SquishRunnerProcess::Continue: return "continue\n";
    case SquishRunnerProcess::Exit: return "exit\n";
    case SquishRunnerProcess::Next: return "next\n";
    case SquishRunnerProcess::PrintVariables: return "print variables\n";
    case SquishRunnerProcess::Quit: return "quit\n";
    case SquishRunnerProcess::Return: return "return\n";
    case SquishRunnerProcess::Step: return "step\n";
    }
    return {};
}

void SquishRunnerProcess::writeCommand(RunnerCommand cmd)
{
    const QString command = cmdToString(cmd);
    if (!command.isEmpty())
        m_process.write(command);
}

void SquishRunnerProcess::requestExpanded(const QString &variableName)
{
    m_process.write("print variables +" + variableName + "\n");
}

// FIXME: add/removal of breakpoints while debugging not handled yet
// FIXME: enabled state of breakpoints
Utils::Links SquishRunnerProcess::setBreakpoints(const QString &scriptExtension)
{
    Utils::Links setBPs;
    using namespace Debugger::Internal;
    const GlobalBreakpoints globalBPs  = BreakpointManager::globalBreakpoints();
    for (const GlobalBreakpoint &gb : globalBPs) {
        if (!gb->isEnabled())
            continue;
        const Utils::FilePath filePath = Utils::FilePath::fromUserInput(
                    gb->data(BreakpointFileColumn, Qt::DisplayRole).toString());
        auto fileName = filePath.canonicalPath().toUserOutput();
        if (fileName.isEmpty())
            continue;
        if (!fileName.endsWith(scriptExtension))
            continue;

        // mask backslashes and spaces
        fileName.replace('\\', "\\\\");
        fileName.replace(' ', "\\x20");
        auto line = gb->data(BreakpointLineColumn, Qt::DisplayRole).toInt();
        QString cmd = "break ";
        cmd.append(fileName);
        cmd.append(':');
        cmd.append(QString::number(line));
        cmd.append('\n');
        qCInfo(runnerLOG).noquote().nospace() << "Setting breakpoint: '" << cmd << "'";
        m_process.write(cmd);
        setBPs.append({filePath, line});
    }
    return setBPs;
}

} // namespace Squish::Internal
