/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "dpastedotcomprotocol.h"

#include <coreplugin/messagemanager.h>

#include <QNetworkReply>
#include <QUrl>

namespace CodePaster {

static QString baseUrl() { return QString("http://dpaste.com"); }
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
        bool publicPaste,
        const QString &username,
        const QString &comment,
        const QString &description
        )
{
    Q_UNUSED(publicPaste)
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

bool DPasteDotComProtocol::checkConfiguration(QString *errorMessage)
{
    if (!m_hostKnownOk)
        m_hostKnownOk = httpStatus(baseUrl(), errorMessage);
    return m_hostKnownOk;
}

void DPasteDotComProtocol::reportError(const QString &message)
{
    const QString fullMessage = tr("%1: %2").arg(protocolName(), message);
    Core::MessageManager::write(fullMessage, Core::MessageManager::ModeSwitch);
}

} // namespace CodePaster
