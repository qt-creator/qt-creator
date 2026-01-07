// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "urlopenprotocol.h"

#include <utils/networkaccessmanager.h>

#include <QtTaskTree/QNetworkReplyWrapper>

using namespace QtTaskTree;

namespace CodePaster {

UrlOpenProtocol::UrlOpenProtocol() : Protocol({"Open URL"})
{}

ExecutableItem UrlOpenProtocol::fetchRecipe(const QString &id, const FetchHandler &handler) const
{
    const auto onSetup = [id](QNetworkReplyWrapper &task) {
        task.setNetworkAccessManager(Utils::NetworkAccessManager::instance());
        task.setRequest(QNetworkRequest(QUrl(id)));
    };
    const auto onDone = [this, handler](const QNetworkReplyWrapper &task, DoneWith result) {
        QNetworkReply *reply = task.reply();
        if (result == DoneWith::Error) {
            reportError(reply->errorString());
            return;
        }
        if (handler)
            handler(reply->url().toString(), QString::fromUtf8(reply->readAll()));
    };

    return QNetworkReplyWrapperTask(onSetup, onDone);
}

} // CodePaster
