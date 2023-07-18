// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "server.h"

using namespace Utils;

namespace Nim::Suggest {

NimSuggestServer::NimSuggestServer(QObject *parent) : QObject(parent)
{
    connect(&m_process, &Process::done, this, &NimSuggestServer::onDone);
    connect(&m_process, &Process::readyReadStandardOutput, this,
            &NimSuggestServer::onStandardOutputAvailable);
}

bool NimSuggestServer::start(const FilePath &executablePath, const FilePath &projectFilePath)
{
    if (!executablePath.exists()) {
        qWarning() << "NimSuggest executable path" << executablePath << "does not exist";
        return false;
    }

    if (!projectFilePath.exists()) {
        qWarning() << "Project file" << projectFilePath << "doesn't exist";
        return false;
    }

    stop();
    m_executablePath = executablePath;
    m_projectFilePath = projectFilePath;
    m_process.setCommand({executablePath, {"--epc", m_projectFilePath.path()}});
    m_process.start();
    return true;
}

void NimSuggestServer::stop()
{
    m_process.close();
    clearState();
}

quint16 NimSuggestServer::port() const
{
    return m_port;
}

void NimSuggestServer::onStandardOutputAvailable()
{
    if (!m_portAvailable) {
        const QString output = m_process.readAllStandardOutput();
        m_port = static_cast<uint16_t>(output.toUInt());
        m_portAvailable = true;
        emit started();
    } else {
        qDebug() << m_process.readAllRawStandardOutput();
    }
}

void NimSuggestServer::onDone()
{
    clearState();
    emit done();
}

void NimSuggestServer::clearState()
{
    m_portAvailable = false;
    m_port = 0;
}

} // namespace Nim::Suggest
