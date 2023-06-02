// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientinterface.h"

#include "languageclienttr.h"

#include <QLoggingCategory>

using namespace LanguageServerProtocol;
using namespace Utils;

static Q_LOGGING_CATEGORY(LOGLSPCLIENTV, "qtc.languageclient.messages", QtWarningMsg);

namespace LanguageClient {

BaseClientInterface::BaseClientInterface()
{
    m_buffer.open(QIODevice::ReadWrite | QIODevice::Append);
}

BaseClientInterface::~BaseClientInterface()
{
    m_buffer.close();
}

void BaseClientInterface::sendMessage(const JsonRpcMessage message)
{
    const BaseMessage baseMessage = message.toBaseMessage();
    sendData(baseMessage.header());
    sendData(baseMessage.content);
}

void BaseClientInterface::resetBuffer()
{
    m_buffer.close();
    m_buffer.setData(nullptr);
    m_buffer.open(QIODevice::ReadWrite | QIODevice::Append);
}

void BaseClientInterface::parseData(const QByteArray &data)
{
    const qint64 preWritePosition = m_buffer.pos();
    qCDebug(parseLog) << "parse buffer pos: " << preWritePosition;
    qCDebug(parseLog) << "  data: " << data;
    if (!m_buffer.atEnd())
        m_buffer.seek(preWritePosition + m_buffer.bytesAvailable());
    m_buffer.write(data);
    m_buffer.seek(preWritePosition);
    while (!m_buffer.atEnd()) {
        QString parseError;
        BaseMessage::parse(&m_buffer, parseError, m_currentMessage);
        qCDebug(parseLog) << "  complete: " << m_currentMessage.isComplete();
        qCDebug(parseLog) << "  length: " << m_currentMessage.contentLength;
        qCDebug(parseLog) << "  content: " << m_currentMessage.content;
        if (!parseError.isEmpty())
            emit error(parseError);
        if (!m_currentMessage.isComplete())
            break;
        parseCurrentMessage();
    }
    if (m_buffer.atEnd()) {
        m_buffer.close();
        m_buffer.setData(nullptr);
        m_buffer.open(QIODevice::ReadWrite | QIODevice::Append);
    }
}

void BaseClientInterface::parseCurrentMessage()
{
    if (m_currentMessage.mimeType == JsonRpcMessage::jsonRpcMimeType()) {
        emit messageReceived(JsonRpcMessage(m_currentMessage));
    } else {
        emit error(Tr::tr("Cannot handle MIME type \"%1\" of message.")
                       .arg(QString::fromUtf8(m_currentMessage.mimeType)));
    }
    m_currentMessage = BaseMessage();
}

StdIOClientInterface::StdIOClientInterface()
    : m_logFile("lspclient.XXXXXX.log")
{
    m_logFile.setAutoRemove(false);
    m_logFile.open();
}

StdIOClientInterface::~StdIOClientInterface()
{
    delete m_process;
}

void StdIOClientInterface::startImpl()
{
    if (m_process) {
        QTC_CHECK(!m_process->isRunning());
        delete m_process;
    }
    m_process = new Process;
    m_process->setProcessMode(ProcessMode::Writer);
    connect(m_process, &Process::readyReadStandardError,
            this, &StdIOClientInterface::readError);
    connect(m_process, &Process::readyReadStandardOutput,
            this, &StdIOClientInterface::readOutput);
    connect(m_process, &Process::started, this, &StdIOClientInterface::started);
    connect(m_process, &Process::done, this, [this] {
        m_logFile.flush();
        if (m_process->result() != ProcessResult::FinishedWithSuccess)
            emit error(QString("%1 (see logs in \"%2\")")
                           .arg(m_process->exitMessage())
                           .arg(m_logFile.fileName()));
        emit finished();
    });
    m_logFile.write(QString("Starting server: %1\nOutput:\n\n").arg(m_cmd.toUserOutput()).toUtf8());
    m_process->setCommand(m_cmd);
    m_process->setWorkingDirectory(m_workingDirectory);
    if (m_env.hasChanges())
        m_process->setEnvironment(m_env);
    m_process->start();
}

void StdIOClientInterface::setCommandLine(const CommandLine &cmd)
{
    m_cmd = cmd;
}

void StdIOClientInterface::setWorkingDirectory(const FilePath &workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

void StdIOClientInterface::setEnvironment(const Environment &environment)
{
    m_env = environment;
}

FilePath StdIOClientInterface::serverDeviceTemplate() const
{
    return m_cmd.executable();
}

void StdIOClientInterface::sendData(const QByteArray &data)
{
    if (!m_process || m_process->state() != QProcess::Running) {
        emit error(Tr::tr("Cannot send data to unstarted server %1")
            .arg(m_cmd.toUserOutput()));
        return;
    }
    qCDebug(LOGLSPCLIENTV) << "StdIOClient send data:";
    qCDebug(LOGLSPCLIENTV).noquote() << data;
    m_process->writeRaw(data);
}

void StdIOClientInterface::readError()
{
    QTC_ASSERT(m_process, return);

    const QByteArray stdErr = m_process->readAllRawStandardError();
    m_logFile.write(stdErr);

    qCDebug(LOGLSPCLIENTV) << "StdIOClient std err:\n";
    qCDebug(LOGLSPCLIENTV).noquote() << stdErr;
}

void StdIOClientInterface::readOutput()
{
    QTC_ASSERT(m_process, return);
    const QByteArray &out = m_process->readAllRawStandardOutput();
    qCDebug(LOGLSPCLIENTV) << "StdIOClient std out:\n";
    qCDebug(LOGLSPCLIENTV).noquote() << out;
    parseData(out);
}

} // namespace LanguageClient
