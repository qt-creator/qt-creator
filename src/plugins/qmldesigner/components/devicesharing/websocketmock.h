// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QT_WEBSOCKET_ENABLED
#include <QWebSocket>
#else
#include <QAbstractSocket>

// QWebSocket mock.
// It is used to avoid linking against QtWebSockets.
namespace QWebSocketProtocol {
enum CloseCode { CloseCodeNormal = 1000 };
enum Version { Unknown = 0, Version13 = 13 };
} // namespace QWebSocketProtocol

class QWebSocket : public QObject
{
    Q_OBJECT
public:
    QWebSocket() = default;
    ~QWebSocket() = default;

    void setOutgoingFrameSize(int) {}
    void setParent(QObject *) {}
    void open(const QUrl &) {}
    void close() {}
    void close(QWebSocketProtocol::CloseCode, const QString &) {}
    void abort() {}
    void flush() {}
    void ping() {}
    bool isValid() {return true;}
    QAbstractSocket::SocketState state() {return QAbstractSocket::ConnectedState;}
    void sendTextMessage(const QString &){}
    void sendBinaryMessage(const QByteArray &){}

signals:
    void pong(quint64, const QByteArray &);
    void textMessageReceived(const QString &);
    void disconnected();
    void connected();
};

#endif // QT_WEBSOCKETS_LIB
