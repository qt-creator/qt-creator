/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "languageclientinterface.h"

#include "languageclientsettings.h"

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

void BaseClientInterface::sendContent(const IContent &content)
{
    const BaseMessage message = content.toBaseMessage();
    sendData(message.header());
    sendData(message.content);
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
        emit contentReceived(JsonRpcMessage(m_currentMessage));
    } else {
        emit error(tr("Cannot handle mimetype of message %1")
                       .arg(QString::fromUtf8(m_currentMessage.mimeType)));
    }
    m_currentMessage = BaseMessage();
}

StdIOClientInterface::StdIOClientInterface() {}

StdIOClientInterface::~StdIOClientInterface()
{
    if (m_process)
        m_process->stopProcess();
    delete m_process;
}

void StdIOClientInterface::startImpl()
{
    m_process = new Utils::QtcProcess;
    m_process->setProcessMode(ProcessMode::Writer);
    connect(m_process, &QtcProcess::readyReadStandardError,
            this, &StdIOClientInterface::readError);
    connect(m_process, &QtcProcess::readyReadStandardOutput,
            this, &StdIOClientInterface::readOutput);
    connect(m_process, &QtcProcess::finished, this, &StdIOClientInterface::onProcessFinished);
    connect(m_process, &QtcProcess::started, this, &StdIOClientInterface::started);
    m_process->setCommand(m_cmd);
    m_process->setWorkingDirectory(m_workingDirectory);
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

void StdIOClientInterface::sendData(const QByteArray &data)
{
    if (!m_process || m_process->state() != QProcess::Running) {
        emit error(tr("Cannot send data to unstarted server %1")
            .arg(m_cmd.toUserOutput()));
        return;
    }
    qCDebug(LOGLSPCLIENTV) << "StdIOClient send data:";
    qCDebug(LOGLSPCLIENTV).noquote() << data;
    m_process->writeRaw(data);
}

void StdIOClientInterface::onProcessFinished()
{
    QTC_ASSERT(m_process, return);
    if (m_process->exitStatus() == QProcess::CrashExit)
        emit error(tr("Crashed with exit code %1: %2")
                       .arg(m_process->exitCode()).arg(m_process->errorString()));
    emit finished();
}

void StdIOClientInterface::readError()
{
    QTC_ASSERT(m_process, return);
    qCDebug(LOGLSPCLIENTV) << "StdIOClient std err:\n";
    qCDebug(LOGLSPCLIENTV).noquote() << m_process->readAllStandardError();
}

void StdIOClientInterface::readOutput()
{
    QTC_ASSERT(m_process, return);
    const QByteArray &out = m_process->readAllStandardOutput();
    qCDebug(LOGLSPCLIENTV) << "StdIOClient std out:\n";
    qCDebug(LOGLSPCLIENTV).noquote() << out;
    parseData(out);
}

} // namespace LanguageClient
