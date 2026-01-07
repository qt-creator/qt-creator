// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dpastedotcomprotocol.h"

#include <coreplugin/messagemanager.h>

#include <utils/networkaccessmanager.h>

#include <QtTaskTree/QNetworkReplyWrapper>

using namespace QtTaskTree;
using namespace Utils;

namespace CodePaster {

static QString baseUrl() { return QString("https://dpaste.com"); }
static QString apiUrl() { return baseUrl() + "/api/v2/"; }

DPasteDotComProtocol::DPasteDotComProtocol()
    : Protocol({protocolName(), Capability::PostDescription | Capability::PostUserName})
{}

QString DPasteDotComProtocol::protocolName() { return QString("DPaste.Com"); }

ExecutableItem DPasteDotComProtocol::fetchRecipe(const QString &id,
                                                 const FetchHandler &handler) const
{
    const Storage<std::optional<QString>> storage;

    const auto onGetSetup = [id](QNetworkReplyWrapper &task) {
        task.setNetworkAccessManager(NetworkAccessManager::instance());
        const QUrl url(baseUrl() + '/' + id + ".txt");
        task.setRequest(QNetworkRequest(url));
    };
    const auto onGetDone = [this, id, storage, handler](const QNetworkReplyWrapper &task, DoneWith result) {
        QNetworkReply *reply = task.reply();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 300 && status <= 308 && status != 306) {
            const QString location = QString::fromUtf8(reply->rawHeader("Location"));
            *storage = location;
            if (status == 301 || status == 308) {
                const QString m = QString("HTTP redirect (%1) to \"%2\"").arg(status).arg(location);
                Core::MessageManager::writeSilently(m);
            }
            return true;
        }
        if (result == DoneWith::Error) {
            reportError(reply->errorString());
            return false;
        }
        if (handler)
            handler(name() + ": " + id, QString::fromUtf8(reply->readAll()));
        return true;
    };

    const auto onRedirectedGetSetup = [storage](QNetworkReplyWrapper &task) {
        if (!*storage)
            return SetupResult::StopWithSuccess;

        task.setNetworkAccessManager(NetworkAccessManager::instance());
        task.setRequest(QNetworkRequest(**storage));
        return SetupResult::Continue;
    };
    const auto onRedirectedGetDone = [this, id, handler](const QNetworkReplyWrapper &task, DoneWith result) {
        QNetworkReply *reply = task.reply();
        if (result == DoneWith::Error) {
            reportError(reply->errorString());
            return;
        }
        if (handler)
            handler(name() + ": " + id, QString::fromUtf8(reply->readAll()));
    };

    return Group {
        storage,
        QNetworkReplyWrapperTask(onGetSetup, onGetDone),
        QNetworkReplyWrapperTask(onRedirectedGetSetup, onRedirectedGetDone)
    };
}

static QByteArray typeToString(ContentType type)
{
    switch (type) {
    case C:          return "c";
    case Cpp:        return "cpp";
    case Diff:       return "diff";
    case JavaScript: return "js";
    case Text:       return "text";
    case Xml:        return "xml";
    }
    return {};
}

void DPasteDotComProtocol::paste(const PasteInputData &inputData)
{
    // See http://dpaste.com/api/v2/
    QByteArray data;
    data += "content=" + QUrl::toPercentEncoding(fixNewLines(inputData.text));
    data += "&expiry_days=" + QByteArray::number(inputData.expiryDays);
    data += "&syntax=" + typeToString(inputData.ct);
    data += "&title=" + QUrl::toPercentEncoding(inputData.description);
    data += "&poster=" + QUrl::toPercentEncoding(inputData.username);

    QNetworkReply * const reply = httpPost(apiUrl(), data);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QString data;
        if (reply->error()) {
            reportError(reply->errorString()); // FIXME: Why can't we properly emit an error here?
            reportError(QString::fromUtf8(reply->readAll()));
        } else {
            data = QString::fromUtf8(reply->readAll());
            if (!data.startsWith(baseUrl())) {
                reportError(data);
                data.clear();
            }
        }
        reply->deleteLater();
        emit pasteDone(data);
    });
}

} // CodePaster
