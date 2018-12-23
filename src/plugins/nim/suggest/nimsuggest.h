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

    std::shared_ptr<SugRequest> sug(const QString &filename, int line, int column,
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
    void onServerCrashed();
    void onServerFinished();

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
