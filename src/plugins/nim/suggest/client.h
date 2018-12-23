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

    std::shared_ptr<SugRequest> sug(const QString &nimFile,
                                    int line, int column,
                                    const QString &dirtyFile);

signals:
    void connected();
    void disconnected();

private:
    void clear();
    void onDisconnectedFromServer();
    void onReadyRead();
    void parsePayload(const char *payload, std::size_t size);

    QTcpSocket m_socket;
    quint16 m_port;
    std::unordered_map<quint64, std::weak_ptr<BaseNimSuggestClientRequest>> m_requests;
    std::vector<QString> m_lines;
    std::vector<char> m_readBuffer;
    quint64 m_lastMessageId = 0;
};

} // namespace Suggest
} // namespace Nim
