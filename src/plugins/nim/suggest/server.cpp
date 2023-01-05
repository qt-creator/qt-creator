// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "server.h"

using namespace Utils;

namespace Nim {
namespace Suggest {

NimSuggestServer::NimSuggestServer(QObject *parent) : QObject(parent)
{
    connect(&m_process, &QtcProcess::done, this, &NimSuggestServer::onDone);
    connect(&m_process, &QtcProcess::readyReadStandardOutput, this,
            &NimSuggestServer::onStandardOutputAvailable);
}

QString NimSuggestServer::executablePath() const
{
    return m_executablePath;
}

bool NimSuggestServer::start(const QString &executablePath,
                             const QString &projectFilePath)
{
    if (!QFile::exists(executablePath)) {
        qWarning() << "NimSuggest executable path" << executablePath << "does not exist";
        return false;
    }

    if (!QFile::exists(projectFilePath)) {
        qWarning() << "Project file" << projectFilePath << "doesn't exist";
        return false;
    }

    stop();
    m_executablePath = executablePath;
    m_projectFilePath = projectFilePath;
    m_process.setCommand({FilePath::fromString(executablePath), {"--epc", m_projectFilePath}});
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

QString NimSuggestServer::projectFilePath() const
{
    return m_projectFilePath;
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

} // namespace Suggest
} // namespace Nim
