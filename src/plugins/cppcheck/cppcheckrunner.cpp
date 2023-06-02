// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcheckrunner.h"
#include "cppchecktool.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <coreplugin/messagemanager.h>

using namespace Utils;

namespace Cppcheck::Internal {

CppcheckRunner::CppcheckRunner(CppcheckTool &tool) : m_tool(tool)
{
    if (HostOsInfo::hostOs() == OsTypeLinux) {
        Process getConf;
        getConf.setCommand({"getconf", {"ARG_MAX"}});
        getConf.start();
        getConf.waitForFinished(2000);
        const QByteArray argMax = getConf.readAllRawStandardOutput().replace("\n", "");
        m_maxArgumentsLength = std::max(argMax.toInt(), m_maxArgumentsLength);
    }

    m_process.setStdOutLineCallback([this](const QString &line) {
        m_tool.parseOutputLine(line);
    });
    m_process.setStdErrLineCallback([this](const QString &line) {
       m_tool.parseErrorLine(line);
    });

    connect(&m_process, &Process::started, &m_tool, &CppcheckTool::startParsing);
    connect(&m_process, &Process::done, this, &CppcheckRunner::handleDone);

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

void CppcheckRunner::reconfigure(const FilePath &binary, const QString &arguments)
{
    m_binary = binary;
    m_arguments = arguments;
}

void CppcheckRunner::addToQueue(const FilePaths &files,
                                const QString &additionalArguments)
{
    FilePaths &existing = m_queue[additionalArguments];
    if (existing.isEmpty()) {
        existing = files;
    } else {
        std::copy_if(files.cbegin(), files.cend(), std::back_inserter(existing),
                     [&existing](const FilePath &file) { return !existing.contains(file); });
    }

    if (m_process.isRunning()) {
        stop(existing);
        return;
    }

    m_queueTimer.start();
}

void CppcheckRunner::stop(const FilePaths &files)
{
    if (!m_process.isRunning())
        return;

    if (files.isEmpty() || m_currentFiles == files)
        m_process.stop();
}

void CppcheckRunner::removeFromQueue(const FilePaths &files)
{
    if (m_queue.isEmpty())
        return;

    if (files.isEmpty()) {
        m_queue.clear();
    } else {
        for (auto it = m_queue.begin(), end = m_queue.end(); it != end;) {
            for (const FilePath &file : files)
                it.value().removeOne(file);
            it = !it.value().isEmpty() ? ++it : m_queue.erase(it);
        }
    }
}

const FilePaths &CppcheckRunner::currentFiles() const
{
    return m_currentFiles;
}

QString CppcheckRunner::currentCommand() const
{
    return m_process.commandLine().toUserOutput();
}

void CppcheckRunner::checkQueued()
{
    if (m_queue.isEmpty() || !m_binary.isExecutableFile())
        return;

    FilePaths files = m_queue.begin().value();
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

    m_process.setCommand(CommandLine(m_binary, arguments, CommandLine::Raw));
    m_process.start();
}

void CppcheckRunner::handleDone()
{
    if (m_process.result() == ProcessResult::FinishedWithSuccess)
        m_tool.finishParsing();
    else
        Core::MessageManager::writeSilently(m_process.exitMessage());

    m_currentFiles.clear();
    m_process.close();

    if (!m_queue.isEmpty())
        checkQueued();
}

} // Cppcheck::Internal
