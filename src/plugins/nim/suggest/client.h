// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QString>
#include <QTcpSocket>

#include <memory>
#include <unordered_map>
#include <vector>

#include "clientrequests.h"

namespace Nim {
namespace Suggest {

class NimSuggestClient : public QObject
{
    Q_OBJECT

public:
    NimSuggestClient(QObject *parent = nullptr);

    bool connectToServer(quint16 port);

    bool disconnectFromServer();

    std::shared_ptr<NimSuggestClientRequest> sug(const QString &nimFile,
                                                 int line, int column,
                                                 const QString &dirtyFile);

    std::shared_ptr<NimSuggestClientRequest> def(const QString &nimFile,
                                                 int line, int column,
                                                 const QString &dirtyFile);

signals:
    void connected();
    void disconnected();

private:
    std::shared_ptr<NimSuggestClientRequest> sendRequest(const QString &type,
                                                         const QString &nimFile,
                                                         int line, int column,
                                                         const QString &dirtyFile);

    void clear();
    void onDisconnectedFromServer();
    void onReadyRead();
    void parsePayload(const char *payload, std::size_t size);

    QTcpSocket m_socket;
    quint16 m_port;
    std::unordered_map<quint64, std::weak_ptr<NimSuggestClientRequest>> m_requests;
    std::vector<QString> m_lines;
    std::vector<char> m_readBuffer;
    quint64 m_lastMessageId = 0;
};

} // namespace Suggest
} // namespace Nim
