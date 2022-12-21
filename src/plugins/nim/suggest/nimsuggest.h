// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "client.h"
#include "server.h"

namespace Nim {
namespace Suggest {

class NimSuggest : public QObject
{
    Q_OBJECT

public:
    NimSuggest(QObject *parent = nullptr);

    QString projectFile() const;
    void setProjectFile(const QString &file);

    QString executablePath() const;
    void setExecutablePath(const QString &path);

    bool isReady() const;

    std::shared_ptr<NimSuggestClientRequest> sug(const QString &filename, int line, int column,
                                                 const QString &dirtyFilename);

    std::shared_ptr<NimSuggestClientRequest> def(const QString &filename, int line, int column,
                                                 const QString &dirtyFilename);

signals:
    void readyChanged(bool ready);
    void projectFileChanged(const QString &projectFile);
    void executablePathChanged(const QString &executablePath);

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
    QString m_projectFile;
    QString m_executablePath;
    NimSuggestServer m_server;
    NimSuggestClient m_client;
};

} // namespace Suggest
} // namespace Nim
