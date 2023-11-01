// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimsuggest.h"

using namespace Utils;

namespace Nim::Suggest {

NimSuggest::NimSuggest(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &NimSuggestServer::started, this, &NimSuggest::onServerStarted);
    connect(&m_server, &NimSuggestServer::done, this, &NimSuggest::onServerDone);

    connect(&m_client, &NimSuggestClient::disconnected, this, &NimSuggest::onClientDisconnected);
    connect(&m_client, &NimSuggestClient::connected, this, &NimSuggest::onClientConnected);
}

FilePath NimSuggest::projectFile() const
{
    return m_projectFile;
}

void NimSuggest::setProjectFile(const Utils::FilePath &file)
{
    if (m_projectFile == file)
        return;

    m_projectFile = file;
    emit projectFileChanged(file);

    restart();
}

FilePath NimSuggest::executablePath() const
{
    return m_executablePath;
}

void NimSuggest::setExecutablePath(const FilePath &path)
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

std::shared_ptr<NimSuggestClientRequest> NimSuggest::sug(const QString &filename, int line, int column,
                                                         const QString &dirtyFilename)
{
    return m_ready ? m_client.sug(filename, line, column, dirtyFilename) : nullptr;
}

std::shared_ptr<NimSuggestClientRequest> NimSuggest::def(const QString &filename, int line, int column, const QString &dirtyFilename)
{
    return m_ready ? m_client.def(filename, line, column, dirtyFilename) : nullptr;
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
    m_server.stop();
}

void NimSuggest::startServer()
{
    if (!m_projectFile.isEmpty() && !m_executablePath.isEmpty())
        m_server.start(m_executablePath, m_projectFile);
}

void NimSuggest::onServerStarted()
{
    setServerReady(true);
    connectClient();
}

void NimSuggest::onServerDone()
{
    setServerReady(false);
    disconnectClient();
    restart();
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

} // Nim::Suggest
