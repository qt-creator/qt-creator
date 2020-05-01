/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
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

#include "cppcheckrunner.h"
#include "cppchecktool.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <coreplugin/messagemanager.h>

using namespace Utils;

namespace Cppcheck {
namespace Internal {

CppcheckRunner::CppcheckRunner(CppcheckTool &tool) :
    m_tool(tool),
    m_process(new Utils::QtcProcess(this))
{
    if (Utils::HostOsInfo::hostOs() == Utils::OsTypeLinux) {
        QProcess getConf;
        getConf.start("getconf", {"ARG_MAX"});
        getConf.waitForFinished(2000);
        const QByteArray argMax = getConf.readAllStandardOutput().replace("\n", "");
        m_maxArgumentsLength = std::max(argMax.toInt(), m_maxArgumentsLength);
    }

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &CppcheckRunner::readOutput);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &CppcheckRunner::readError);
    connect(m_process, &QProcess::started,
            this, &CppcheckRunner::handleStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CppcheckRunner::handleFinished);

    m_queueTimer.setSingleShot(true);
    const int checkDelayInMs = 200;
    m_queueTimer.setInterval(checkDelayInMs);
    connect(&m_queueTimer, &QTimer::timeout,
            this, &CppcheckRunner::checkQueued);
}

CppcheckRunner::~CppcheckRunner()
{
    stop();
    m_queueTimer.stop();
}

void CppcheckRunner::reconfigure(const QString &binary, const QString &arguments)
{
    m_binary = binary;
    m_arguments = arguments;
}

void CppcheckRunner::addToQueue(const Utils::FilePaths &files,
                                const QString &additionalArguments)
{
    Utils::FilePaths &existing = m_queue[additionalArguments];
    if (existing.isEmpty()) {
        existing = files;
    } else {
        std::copy_if(files.cbegin(), files.cend(), std::back_inserter(existing),
                     [&existing](const Utils::FilePath &file) { return !existing.contains(file); });
    }

    if (m_isRunning) {
        stop(existing);
        return;
    }

    m_queueTimer.start();
}

void CppcheckRunner::stop(const Utils::FilePaths &files)
{
    if (!m_isRunning)
        return;

    if (files.isEmpty() || m_currentFiles == files)
        m_process->kill();
}

void CppcheckRunner::removeFromQueue(const Utils::FilePaths &files)
{
    if (m_queue.isEmpty())
        return;

    if (files.isEmpty()) {
        m_queue.clear();
    } else {
        for (auto it = m_queue.begin(), end = m_queue.end(); it != end;) {
            for (const Utils::FilePath &file : files)
                it.value().removeOne(file);
            it = !it.value().isEmpty() ? ++it : m_queue.erase(it);
        }
    }
}

const Utils::FilePaths &CppcheckRunner::currentFiles() const
{
    return m_currentFiles;
}

QString CppcheckRunner::currentCommand() const
{
    return m_process->program() + ' ' +
            m_process->arguments().join(' ');
}

void CppcheckRunner::checkQueued()
{
    if (m_queue.isEmpty() || m_binary.isEmpty())
        return;

    Utils::FilePaths files = m_queue.begin().value();
    QString arguments = m_arguments + ' ' + m_queue.begin().key();
    m_currentFiles.clear();
    int argumentsLength = arguments.length();
    while (!files.isEmpty()) {
        argumentsLength += files.first().toString().size() + 1; // +1 for separator
        if (argumentsLength >= m_maxArgumentsLength)
            break;
        m_currentFiles.push_back(files.first());
        arguments += ' ' + files.first().toString();
        files.pop_front();
    }

    if (files.isEmpty())
        m_queue.erase(m_queue.begin());
    else
        m_queue.begin().value() = files;

    m_process->setCommand(CommandLine(FilePath::fromString(m_binary), arguments, CommandLine::Raw));
    m_process->start();
}

void CppcheckRunner::readOutput()
{
    if (!m_isRunning) // workaround for QTBUG-30929
        handleStarted();

    m_process->setReadChannel(QProcess::StandardOutput);

    while (!m_process->atEnd() && m_process->canReadLine()) {
        QString line = QString::fromUtf8(m_process->readLine());
        if (line.endsWith('\n'))
            line.chop(1);
        m_tool.parseOutputLine(line);
    }
}

void CppcheckRunner::readError()
{
    if (!m_isRunning) // workaround for QTBUG-30929
        handleStarted();

    m_process->setReadChannel(QProcess::StandardError);

    while (!m_process->atEnd() && m_process->canReadLine()) {
        QString line = QString::fromUtf8(m_process->readLine());
        if (line.endsWith('\n'))
            line.chop(1);
        m_tool.parseErrorLine(line);
    }
}

void CppcheckRunner::handleStarted()
{
    if (m_isRunning)
        return;

    m_isRunning = true;
    m_tool.startParsing();
}

void CppcheckRunner::handleFinished(int)
{
    if (m_process->error() != QProcess::FailedToStart) {
        readOutput();
        readError();
        m_tool.finishParsing();
    } else {
        const QString message = tr("Cppcheck failed to start: \"%1\".").arg(currentCommand());
        Core::MessageManager::write(message, Core::MessageManager::Silent);
    }
    m_currentFiles.clear();
    m_process->close();
    m_isRunning = false;

    if (!m_queue.isEmpty())
        checkQueued();
}

} // namespace Internal
} // namespace Cppcheck
