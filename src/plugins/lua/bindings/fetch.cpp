// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luaqttypes.h"

#include <utils/networkaccessmanager.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaEnum>
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
    LuaEngine::registerProvider(
        "Fetch", [](sol::state_view lua) -> sol::object {
            sol::table async = lua.script("return require('async')", "_fetch_").get<sol::table>();
            sol::function wrap = async["wrap"];

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

            fetch["fetch_cb"] = [](const sol::table &options,
                                   const sol::function &callback,
                                   const sol::this_state &thisState) {
                auto url = options.get<QString>("url");

                auto method = (options.get_or<QString>("method", "GET")).toLower();
                auto headers = options.get_or<sol::table>("headers", {});
                auto data = options.get_or<QString>("body", {});
                bool convertToTable
                    = options.get<std::optional<bool>>("convertToTable").value_or(false);

                QNetworkRequest request((QUrl(url)));
                if (headers && !headers.empty()) {
                    for (const auto &[k, v] : headers)
                        request.setRawHeader(k.as<QString>().toUtf8(), v.as<QString>().toUtf8());
                }

                QNetworkReply *reply = nullptr;
                if (method == "get")
                    reply = Utils::NetworkAccessManager::instance()->get(request);
                else if (method == "post")
                    reply = Utils::NetworkAccessManager::instance()->post(request, data.toUtf8());
                else
                    throw std::runtime_error("Unknown method: " + method.toStdString());

                if (convertToTable) {
                    QObject::connect(
                        reply,
                        &QNetworkReply::finished,
                        &LuaEngine::instance(),
                        [reply, thisState, callback]() {
                            reply->deleteLater();

                            if (reply->error() != QNetworkReply::NoError) {
                                callback(QString("%1 (%2):\n%3")
                                             .arg(reply->errorString())
                                             .arg(QString::fromLatin1(
                                                 QMetaEnum::fromType<QNetworkReply::NetworkError>()
                                                     .valueToKey(reply->error())))
                                             .arg(QString::fromUtf8(reply->readAll())));
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
                                callback(LuaEngine::toTable(thisState, doc.object()));
                            } else if (doc.isArray()) {
                                callback(LuaEngine::toTable(thisState, doc.array()));
                            } else {
                                sol::state_view lua(thisState);
                                callback(lua.create_table());
                            }
                        });

                } else {
                    QObject::connect(
                        reply, &QNetworkReply::finished, &LuaEngine::instance(), [reply, callback]() {
                            // We don't want the network reply to be deleted by the manager, but
                            // by the Lua GC
                            reply->setParent(nullptr);
                            callback(std::unique_ptr<QNetworkReply>(reply));
                        });
                }
            };

            fetch["fetch"] = wrap(fetch["fetch_cb"]);

            return fetch;
        });
}

} // namespace Lua::Internal
