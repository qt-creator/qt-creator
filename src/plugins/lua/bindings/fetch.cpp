// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luaqttypes.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Lua::Internal {

static QString opToString(QNetworkAccessManager::Operation op)
{
    switch (op) {
    case QNetworkAccessManager::HeadOperation:
        return "HEAD";
    case QNetworkAccessManager::GetOperation:
        return "GET";
    case QNetworkAccessManager::PutOperation:
        return "PUT";
    case QNetworkAccessManager::PostOperation:
        return "POST";

    case QNetworkAccessManager::DeleteOperation:
        return "DELETE";
    case QNetworkAccessManager::CustomOperation:
        return "CUSTOM";
    default:
        return "UNKNOWN";
    }
}

void addFetchModule()
{
    LuaEngine::registerProvider("__fetch", [](sol::state_view lua) -> sol::object {
        sol::table fetch = lua.create_table();

        auto networkReplyType = lua.new_usertype<QNetworkReply>(
            "QNetworkReply",
            "error",
            sol::property([](QNetworkReply *self) -> int { return self->error(); }),
            "readAll",
            [](QNetworkReply *r) { return r->readAll().toStdString(); },
            "__tostring",
            [](QNetworkReply *r) {
                return QString("QNetworkReply(%1 \"%2\") => %3")
                    .arg(opToString(r->operation()))
                    .arg(r->url().toString())
                    .arg(r->error());
            });

        static QNetworkAccessManager networkAccessManager;

        fetch["fetch_cb"] = [](sol::table options, sol::function callback, sol::this_state s) {
            auto url = options.get<QString>("url");

            auto method = (options.get_or<QString>("method", "GET")).toLower();
            auto headers = options.get_or<sol::table>("headers", {});
            auto data = options.get_or<QString>("body", {});
            bool convertToTable = options.get<std::optional<bool>>("convertToTable").value_or(false);

            QNetworkRequest request((QUrl(url)));
            if (headers && !headers.empty()) {
                for (const auto &[k, v] : headers) {
                    request.setRawHeader(k.as<QString>().toUtf8(), v.as<QString>().toUtf8());
                }
            }

            QNetworkReply *reply = nullptr;
            if (method == "get")
                reply = networkAccessManager.get(request);
            else if (method == "post")
                reply = networkAccessManager.post(request, data.toUtf8());
            else
                throw std::runtime_error("Unknown method: " + method.toStdString());

            if (convertToTable) {
                QObject::connect(reply, &QNetworkReply::finished, reply, [reply, s, callback]() {
                    reply->deleteLater();

                    if (reply->error() != QNetworkReply::NoError) {
                        callback(QString("%1 (%2)").arg(reply->errorString()).arg(reply->error()));
                        return;
                    }

                    QByteArray data = reply->readAll();
                    QJsonParseError error;
                    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        callback(error.errorString());
                        return;
                    }
                    if (doc.isObject()) {
                        callback(LuaEngine::toTable(s, doc.object()));
                    } else if (doc.isArray()) {
                        callback(LuaEngine::toTable(s, doc.array()));
                    } else {
                        sol::state_view lua(s);
                        callback(lua.create_table());
                    }
                });

            } else {
                QObject::connect(reply, &QNetworkReply::finished, reply, [reply, callback]() {
                    // We don't want the network reply to be deleted by the manager, but
                    // by the Lua GC
                    reply->setParent(nullptr);
                    callback(std::unique_ptr<QNetworkReply>(reply));
                });
            }
        };

        return fetch;
    });

    LuaEngine::registerProvider("Fetch", [](sol::state_view lua) -> sol::object {
        return lua
            .script(
                R"(
local f = require("__fetch")
local a = require("async")

return {
    fetch_cb = f.fetch_cb,
    fetch = a.wrap(f.fetch_cb)
}
)",
                "_fetch_")
            .get<sol::table>();
    });
}

} // namespace Lua::Internal
