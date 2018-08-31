/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "pastecodedotxyzprotocol.h"

#include <coreplugin/messagemanager.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkReply>
#include <QUrl>

namespace CodePaster {

static QString baseUrl() { return QString("https://pastecode.xyz"); }
static QString apiUrl() { return baseUrl() + "/api"; }

QString PasteCodeDotXyzProtocol::protocolName() { return QString("Pastecode.Xyz"); }

unsigned PasteCodeDotXyzProtocol::capabilities() const
{
    return ListCapability | PostDescriptionCapability | PostUserNameCapability;
}

void PasteCodeDotXyzProtocol::fetch(const QString &id)
{
    QNetworkReply * const reply = httpGet(baseUrl() + "/view/raw/" + id);
    connect(reply, &QNetworkReply::finished, this, [this, id, reply] {
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
    });
}

void PasteCodeDotXyzProtocol::paste(const QString &text, Protocol::ContentType ct, int expiryDays,
                                    const QString &username, const QString &comment,
                                    const QString &description)
{
    QByteArray data;
    data += "text=" + QUrl::toPercentEncoding(fixNewLines(text));
    data += "&expire=" + QUrl::toPercentEncoding(QString::number(expiryDays * 24 * 60));
    data += "&title=" + QUrl::toPercentEncoding(description);
    data += "&name=" + QUrl::toPercentEncoding(username);
    static const auto langValue = [](Protocol::ContentType type) -> QByteArray {
        switch (type) {
        case Protocol::Text: return "text";
        case Protocol::C: return "c";
        case Protocol::Cpp: return "cpp";
        case Protocol::JavaScript: return "javascript";
        case Protocol::Diff: return "diff";
        case Protocol::Xml: return "xml";
        }
        return QByteArray(); // Crutch for compiler.
    };
    data += "&lang=" + langValue(ct);
    Q_UNUSED(comment);

    QNetworkReply * const reply = httpPost(apiUrl() + "/create", data);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QString data;
        if (reply->error()) {
            reportError(reply->errorString()); // FIXME: Why can't we properly emit an error here?
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

void PasteCodeDotXyzProtocol::list()
{
    QNetworkReply * const reply = httpGet(apiUrl() + "/recent");
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        QStringList ids;
        if (reply->error()) {
            reportError(reply->errorString());
        } else {
            QJsonParseError parseError;
            const QJsonDocument jsonData = QJsonDocument::fromJson(reply->readAll(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                reportError(parseError.errorString());
            } else {
                const QJsonArray jsonList = jsonData.array();
                for (auto it = jsonList.constBegin(); it != jsonList.constEnd(); ++it) {
                    const QString id = it->toObject().value("pid").toString();
                    if (!id.isEmpty())
                        ids << id;
                }
            }
        }
        emit listDone(name(), ids);
        reply->deleteLater();
    });
}

bool PasteCodeDotXyzProtocol::checkConfiguration(QString *errorMessage)
{
    if (!m_hostKnownOk)
        m_hostKnownOk = httpStatus(apiUrl(), errorMessage);
    return m_hostKnownOk;
}

void PasteCodeDotXyzProtocol::reportError(const QString &message)
{
    const QString fullMessage = tr("%1: %2").arg(protocolName(), message);
    Core::MessageManager::write(fullMessage, Core::MessageManager::ModeSwitch);
}

} // namespace CodePaster
