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

#include "nimsuggest.h"

namespace Nim {
namespace Suggest {

NimSuggest::NimSuggest(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &NimSuggestServer::started, this, &NimSuggest::onServerStarted);
    connect(&m_server, &NimSuggestServer::crashed, this, &NimSuggest::onServerCrashed);
    connect(&m_server, &NimSuggestServer::finished, this, &NimSuggest::onServerFinished);

    connect(&m_client, &NimSuggestClient::disconnected, this, &NimSuggest::onClientDisconnected);
    connect(&m_client, &NimSuggestClient::connected, this, &NimSuggest::onClientConnected);
}

QString NimSuggest::projectFile() const
{
    return m_projectFile;
}

void NimSuggest::setProjectFile(const QString &file)
{
    if (m_projectFile == file)
        return;

    m_projectFile = file;
    emit projectFileChanged(file);

    restart();
}

QString NimSuggest::executablePath() const
{
    return m_executablePath;
}

void NimSuggest::setExecutablePath(const QString &path)
{
    if (m_executablePath == path)
        return;

    m_executablePath = path;
    emit executablePathChanged(path);

    restart();
}

bool NimSuggest::isReady() const
{
    return m_ready;
}

std::shared_ptr<SugRequest> NimSuggest::sug(const QString &filename, int line, int column,
                                            const QString &dirtyFilename)
{
    return m_ready ? m_client.sug(filename, line, column, dirtyFilename) : nullptr;
}

void NimSuggest::restart()
{
    disconnectClient();
    setClientReady(false);

    stopServer();
    setServerReady(false);

    startServer();
}

void NimSuggest::setReady(bool ready)
{
    if (m_ready == ready)
        return;
    m_ready = ready;
    emit readyChanged(ready);
}

void NimSuggest::setServerReady(bool ready)
{
    if (m_serverReady == ready)
        return;
    m_serverReady = ready;
    setReady(m_clientReady && m_serverReady);
}

void NimSuggest::setClientReady(bool ready)
{
    if (m_clientReady == ready)
        return;
    m_clientReady = ready;
    setReady(m_clientReady && m_serverReady);
}

void NimSuggest::connectClient()
{
    m_client.connectToServer(m_server.port());
}

void NimSuggest::disconnectClient()
{
    m_client.disconnectFromServer();
}

void NimSuggest::stopServer()
{
    m_server.kill();
}

void NimSuggest::startServer()
{
    if (!m_projectFile.isEmpty() && !m_executablePath.isEmpty()) {
        m_server.start(m_executablePath, m_projectFile);
    }
}

void NimSuggest::onServerStarted()
{
    setServerReady(true);
    connectClient();
}

void NimSuggest::onServerCrashed()
{
    setServerReady(false);
    disconnectClient();
    restart();
}

void NimSuggest::onServerFinished()
{
    onServerCrashed();
}

void NimSuggest::onClientConnected()
{
    setClientReady(true);
}

void NimSuggest::onClientDisconnected()
{
    setClientReady(false);
    if (isServerReady())
        connectClient();
}

} // namespace Suggest
} // namespace Nim
