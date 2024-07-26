
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <QLocalSocket>
#include <QTimer>

namespace Lua::Internal {

class LocalSocket : public QLocalSocket
{
public:
    using QLocalSocket::QLocalSocket;
};

void setupLocalSocketModule()
{
    registerProvider("LocalSocket", [](sol::state_view lua) -> sol::object {
        sol::table async = lua.script("return require('async')", "_localsocket_").get<sol::table>();
        sol::function wrap = async["wrap"];

        sol::table result = lua.create_table();

        sol::usertype<LocalSocket> socketType
            = result.new_usertype<LocalSocket>("LocalSocket", sol::no_constructor);

        result["create"] = [lua](const QString &name) {
            auto socket = std::make_unique<LocalSocket>();
            socket->setServerName(name);
            return socket;
        };

        socketType["isConnected"] = [](LocalSocket *socket) {
            return socket->state() == QLocalSocket::ConnectedState;
        };

        socketType["connectToServer_cb"] = [](LocalSocket *socket, sol::function cb) {
            if (socket->state() != QLocalSocket::UnconnectedState)
                throw sol::error("socket is not in UnconnectedState");

            auto connection = new QMetaObject::Connection;
            auto connectionError = new QMetaObject::Connection;

            *connection = QObject::connect(
                socket, &QLocalSocket::connected, socket, [connectionError, connection, cb]() {
                    qDebug() << "CONNECTED";
                    auto res = void_safe_call(cb, true);
                    QTC_CHECK_EXPECTED(res);
                    QObject::disconnect(*connection);
                    delete connection;
                    QObject::disconnect(*connectionError);
                    delete connectionError;
                });
            *connectionError = QObject::connect(
                socket,
                &QLocalSocket::errorOccurred,
                socket,
                [socket, connection, connectionError, cb]() {
                    qDebug() << "CONNECT ERROR";
                    auto res = void_safe_call(cb, false, socket->errorString());
                    QTC_CHECK_EXPECTED(res);
                    QObject::disconnect(*connection);
                    delete connection;
                    QObject::disconnect(*connectionError);
                    delete connectionError;
                });

            socket->connectToServer();
        };

        socketType["connectToServer"]
            = wrap(socketType["connectToServer_cb"].get<sol::function>()).get<sol::function>();

        socketType["write"] = [](LocalSocket *socket, const std::string &data) {
            if (socket->state() != QLocalSocket::ConnectedState)
                throw sol::error("socket is not in ConnectedState");

            return socket->write(data.c_str(), data.size());
        };

        socketType["read_cb"] = [](LocalSocket *socket, sol::function cb) {
            if (socket->state() != QLocalSocket::ConnectedState)
                throw sol::error("socket is not in ConnectedState");

            if (socket->bytesAvailable() > 0) {
                QTimer::singleShot(0, [cb, socket]() {
                    void_safe_call(cb, socket->readAll().toStdString());
                });
                return;
            }

            auto connection = new QMetaObject::Connection;
            *connection = QObject::connect(
                socket, &QLocalSocket::readyRead, socket, [connection, cb, socket]() {
                    auto res = void_safe_call(cb, socket->readAll().toStdString());
                    QTC_CHECK_EXPECTED(res);
                    QObject::disconnect(*connection);
                    delete connection;
                });
        };

        socketType["read"] = wrap(socketType["read_cb"].get<sol::function>()).get<sol::function>();

        socketType["close"] = [](LocalSocket *socket) { socket->close(); };

        return result;
    });
}

} // namespace Lua::Internal
