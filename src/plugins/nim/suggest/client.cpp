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

#include "client.h"

#include "sexprparser.h"

#include <array>

namespace Nim {
namespace Suggest {

NimSuggestClient::NimSuggestClient(QObject *parent) : QObject(parent)
{
    connect(&m_socket, &QTcpSocket::readyRead, this, &NimSuggestClient::onReadyRead);
    connect(&m_socket, &QTcpSocket::connected, this, &NimSuggestClient::connected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &NimSuggestClient::disconnected);
}

bool NimSuggestClient::connectToServer(quint16 port)
{
    m_port = port;
    m_socket.connectToHost("localhost", m_port);
    return true;
}

bool NimSuggestClient::disconnectFromServer()
{
    m_socket.disconnectFromHost();
    clear();
    return true;
}

std::shared_ptr<NimSuggestClientRequest> NimSuggestClient::sug(const QString &nimFile,
                                                               int line, int column,
                                                               const QString &dirtyFile)
{
    return sendRequest(QLatin1String("sug"), nimFile, line, column, dirtyFile);
}

std::shared_ptr<NimSuggestClientRequest> NimSuggestClient::def(const QString &nimFile,
                                                               int line, int column,
                                                               const QString &dirtyFile)
{
    return sendRequest(QLatin1String("def"), nimFile, line, column, dirtyFile);
}

std::shared_ptr<NimSuggestClientRequest> NimSuggestClient::sendRequest(const QString& type,
                                                                       const QString &nimFile,
                                                                       int line, int column,
                                                                       const QString &dirtyFile)
{
    if (!m_socket.isOpen())
        return nullptr;

    auto result = std::make_shared<NimSuggestClientRequest>(m_lastMessageId++);
    m_requests.emplace(result->id(), result);

    QByteArray body = QString(R"((call %1 %2 ("%3" %4 %5 "%6"))\n)")
            .arg(result->id())
            .arg(type)
            .arg(nimFile)
            .arg(line).arg(column)
            .arg(dirtyFile)
            .toUtf8();

    QByteArray length = QString::number(body.size(), 16).rightJustified(6, '0').toUtf8();
    QByteArray message = length + body;

    m_socket.write(message);
    m_socket.waitForBytesWritten();
    return result;
}

void NimSuggestClient::clear()
{
    for (const auto &pair : m_requests) {
        if (auto req = pair.second.lock()) {
            emit req->finished();
        }
    }
    m_lines.clear();
    m_readBuffer.clear();
    m_requests.clear();
    m_lastMessageId = 0;
}

void NimSuggestClient::onDisconnectedFromServer()
{
    clear();
}

void NimSuggestClient::onReadyRead()
{
    std::array<char, 2048> buffer;

    for (;;) {
        qint64 num_read = m_socket.read(buffer.data(), buffer.size());
        m_readBuffer.insert(m_readBuffer.end(), buffer.begin(), buffer.begin() + num_read);
        if (num_read <= 0) {
            break;
        }
    }

    while (m_readBuffer.size() >= 6) {
        const size_t payload_size = QByteArray::fromRawData(m_readBuffer.data(), 6).toUInt(nullptr, 16);
        if (payload_size <= m_readBuffer.size() - 6) {
            parsePayload(m_readBuffer.data() + 6, payload_size);
            m_readBuffer.erase(m_readBuffer.begin(), m_readBuffer.begin() + 6u + payload_size);
        } else {
            break;
        }
    }
}

void NimSuggestClient::parsePayload(const char *payload, std::size_t size)
{
    SExprParser::Node root_ast;
    SExprParser parser(payload, size);
    if (!parser.parse(root_ast))
        return;

    if (root_ast.nodes.at(0).value != "return")
        return;

    const quint64 uid = std::stoull(root_ast.nodes.at(1).value, nullptr, 0);

    auto it = m_requests.find(uid);
    if (it == m_requests.end())
        return;

    auto req = std::dynamic_pointer_cast<NimSuggestClientRequest>((*it).second.lock());
    if (!req)
        return;

    std::vector<Line> suggestions;
    suggestions.reserve(root_ast.nodes.at(2).nodes.size());
    for (const SExprParser::Node &row : root_ast.nodes.at(2).nodes) {
        Line line;
        if (!Line::fromString(line.line_type, row.nodes.at(0).value))
            continue;
        if (!Line::fromString(line.symbol_kind, row.nodes.at(1).value))
            continue;
        line.data.reserve(row.nodes.at(2).nodes.size());
        for (const SExprParser::Node &field : row.nodes.at(2).nodes)
            line.data.push_back(QString::fromStdString(field.value));
        line.abs_path = QString::fromStdString(row.nodes.at(3).value);
        line.symbol_type = QString::fromStdString(row.nodes.at(4).value);
        line.row = std::stoi(row.nodes.at(5).value);
        line.column = std::stoi(row.nodes.at(6).value);
        line.doc = QString::fromStdString(row.nodes.at(7).value);
        suggestions.push_back(std::move(line));
    }
    req->setFinished(std::move(suggestions));

    m_requests.erase(it);
}

} // namespace Suggest
} // namespace Nim
