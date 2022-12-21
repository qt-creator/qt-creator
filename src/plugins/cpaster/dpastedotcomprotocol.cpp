// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dpastedotcomprotocol.h"

#include "cpastertr.h"

#include <coreplugin/messagemanager.h>

#include <QNetworkReply>
#include <QUrl>

namespace CodePaster {

static QString baseUrl() { return QString("https://dpaste.com"); }
static QString apiUrl() { return baseUrl() + "/api/v2/"; }

QString DPasteDotComProtocol::protocolName() { return QString("DPaste.Com"); }

unsigned DPasteDotComProtocol::capabilities() const
{
    return PostDescriptionCapability | PostUserNameCapability;
}

void DPasteDotComProtocol::fetch(const QString &id)
{
    QNetworkReply * const reply = httpGet(baseUrl() + '/' + id + ".txt");
    connect(reply, &QNetworkReply::finished, this, [this, id, reply] {
        fetchFinished(id, reply, false);
    });
}

void DPasteDotComProtocol::fetchFinished(const QString &id, QNetworkReply * const reply,
                                         bool alreadyRedirected)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 300 && status <= 308 && status != 306) {
        if (!alreadyRedirected) {
            const QString location = QString::fromUtf8(reply->rawHeader("Location"));
            if (status == 301 || status == 308) {
                const QString m = QString("HTTP redirect (%1) to \"%2\"").arg(status).arg(location);
                Core::MessageManager::writeSilently(m);
            }
            QNetworkReply * const newRep = httpGet(location);
            connect(newRep, &QNetworkReply::finished, this, [this, id, newRep] {
                fetchFinished(id, newRep, true);
            });
            reply->deleteLater();
            return;
        }
    }
    QString title;
    QString content;
    const bool error = reply->error();
    if (error) {
        content = reply->errorString();
    } else {
        title = name() + ": " + id;
        content = QString::fromUtf8(reply->readAll());
    }
    reply->deleteLater();
    emit fetchDone(title, content, error);
}

static QByteArray typeToString(Protocol::ContentType type)
{
    switch (type) {
    case Protocol::C:
        return "c";
    case Protocol::Cpp:
        return "cpp";
    case Protocol::Diff:
        return "diff";
    case Protocol::JavaScript:
        return "js";
    case Protocol::Text:
        return "text";
    case Protocol::Xml:
        return "xml";
    }
    return {}; // For dumb compilers.
}

void DPasteDotComProtocol::paste(
        const QString &text,
        ContentType ct,
        int expiryDays,
        const QString &username,
        const QString &comment,
        const QString &description
        )
{
    Q_UNUSED(comment)

    // See http://dpaste.com/api/v2/
    QByteArray data;
    data += "content=" + QUrl::toPercentEncoding(fixNewLines(text));
    data += "&expiry_days=" + QByteArray::number(expiryDays);
    data += "&syntax=" + typeToString(ct);
    data += "&title=" + QUrl::toPercentEncoding(description);
    data += "&poster=" + QUrl::toPercentEncoding(username);

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

bool DPasteDotComProtocol::checkConfiguration(QString * /*errorMessage*/)
{
    // we need a 1s gap between requests, so skip status check to avoid failing
    return true;
}

void DPasteDotComProtocol::reportError(const QString &message)
{
    const QString fullMessage = Tr::tr("%1: %2").arg(protocolName(), message);
    Core::MessageManager::writeDisrupting(fullMessage);
}

} // CodePaster
