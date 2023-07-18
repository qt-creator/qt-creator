// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "client.h"
#include "server.h"

#include <utils/filepath.h>


namespace Nim::Suggest {

class NimSuggest : public QObject
{
    Q_OBJECT

public:
    NimSuggest(QObject *parent = nullptr);

    Utils::FilePath projectFile() const;
    void setProjectFile(const Utils::FilePath &file);

    Utils::FilePath executablePath() const;
    void setExecutablePath(const Utils::FilePath &path);

    bool isReady() const;

    std::shared_ptr<NimSuggestClientRequest> sug(const QString &filename, int line, int column,
                                                 const QString &dirtyFilename);

    std::shared_ptr<NimSuggestClientRequest> def(const QString &filename, int line, int column,
                                                 const QString &dirtyFilename);

signals:
    void readyChanged(bool ready);
    void projectFileChanged(const Utils::FilePath &projectFile);
    void executablePathChanged(const Utils::FilePath &executablePath);

private:
    void restart();

    void setReady(bool ready);

    bool isServerReady() const { return m_serverReady; }
    void setServerReady(bool ready);

    bool isClientReady() const { return m_clientReady; }
    void setClientReady(bool ready);

    void connectClient();
    void disconnectClient();

    void stopServer();
    void startServer();

    void onServerStarted();
    void onServerDone();

    void onClientConnected();
    void onClientDisconnected();

    bool m_ready = false;
    bool m_clientReady = false;
    bool m_serverReady = false;
    Utils::FilePath m_projectFile;
    Utils::FilePath m_executablePath;
    NimSuggestServer m_server;
    NimSuggestClient m_client;
};

} // Nim::Suggest
