/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "server.h"

namespace Nim {
namespace Suggest {

NimSuggestServer::NimSuggestServer(QObject *parent) : QObject(parent)
{
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NimSuggestServer::onFinished);
    connect(&m_process, &QProcess::started, this, &NimSuggestServer::onStarted);
    connect(&m_process, &QProcess::readyReadStandardOutput, this,
            &NimSuggestServer::onStandardOutputAvailable);
}

NimSuggestServer::~NimSuggestServer()
{
    kill();
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

    m_port = 0;
    m_executablePath = executablePath;
    m_projectFilePath = projectFilePath;
    m_process.start(executablePath, {"--epc", m_projectFilePath});
    return true;
}

void NimSuggestServer::kill()
{
    disconnect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
               this, &NimSuggestServer::onFinished);
    m_process.kill();
    m_process.waitForFinished();
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

void NimSuggestServer::onStarted()
{
    m_started = true;
}

void NimSuggestServer::onStandardOutputAvailable()
{
    if (m_started && !m_portAvailable) {
        auto output = QString::fromUtf8(m_process.readAllStandardOutput());
        m_port = static_cast<uint16_t>(output.toUInt());
        m_portAvailable = true;
        emit started();
    } else {
        qDebug() << m_process.readAllStandardOutput();
    }
}

void NimSuggestServer::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    clearState();

    Q_UNUSED(exitCode)
    if (exitStatus == QProcess::ExitStatus::CrashExit)
        emit crashed();
    else
        emit finished();
}

void NimSuggestServer::clearState()
{
    m_started = false;
    m_portAvailable = false;
    m_port = 0;
}

} // namespace Suggest
} // namespace Nim
