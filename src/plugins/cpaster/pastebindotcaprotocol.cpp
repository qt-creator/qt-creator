/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "pastebindotcaprotocol.h"

#include <utils/qtcassert.h>

#include <QNetworkReply>
#include <QStringList>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>

static const char urlC[] = "https://pastebin.ca/";
static const char internalUrlC[] = "https://pbin.ca/";
static const char protocolNameC[] = "Pastebin.Ca";

static inline QByteArray expiryValue(int expiryDays)
{
    // pastebin.ca supports 1-3 days, 1-3 weeks, 1-6 months, 1 year
    const int months = expiryDays / 30;
    const int weeks = expiryDays / 7;

    if (expiryDays == 1)
        return "1 day";
    if (expiryDays < 4)
        return QByteArray::number(expiryDays) + " days";
    if (weeks <= 1)
        return "1 week";
    if (weeks <= 3)
        return QByteArray::number(weeks) + " weeks";
    if (months <= 1)
        return "1 month";
    if (months <= 6)
        return QByteArray::number(months) + " months";
    return "1 year"; // using Never makes the post expire after 1 month
}

namespace CodePaster {

QString PasteBinDotCaProtocol::protocolName()
{
    return QLatin1String(protocolNameC);
}

unsigned PasteBinDotCaProtocol::capabilities() const
{
    return ListCapability | PostDescriptionCapability | PostCommentCapability;
}

void PasteBinDotCaProtocol::fetch(const QString &id)
{
    QTC_ASSERT(!m_fetchReply, return);
    const QString url = QLatin1String(internalUrlC);
    const QString rawPostFix = QLatin1String("raw/");
    // Create link as ""http://pbin.ca/raw/[id]"
    // If we get a complete URL, just insert 'raw', else build URL.
    QString link = id;
    if (link.startsWith(url)) {
        const int lastSlashPos = link.lastIndexOf(QLatin1Char('/'));
        if (lastSlashPos != -1)
            link.insert(lastSlashPos + 1, rawPostFix);
    } else {
        link.insert(0, rawPostFix);
        link.insert(0, url);
    }
    m_fetchReply = httpGet(link);
    connect(m_fetchReply, &QNetworkReply::finished,
            this, &PasteBinDotCaProtocol::fetchFinished);
    m_fetchId = id;
}

static QByteArray toTypeId(Protocol::ContentType ct)
{
    if (ct == Protocol::C)
        return QByteArray(1, '3');
    if (ct == Protocol::Cpp)
        return QByteArray(1, '4');
    if (ct == Protocol::JavaScript)
        return QByteArray("27");
    if (ct == Protocol::Diff)
        return QByteArray( "34");
    if (ct == Protocol::Xml)
        return QByteArray("15");
    return QByteArray(1, '1');
}

void PasteBinDotCaProtocol::paste(const QString &text,
                                  ContentType ct, int expiryDays,
                                  const QString &/* username */,
                                  const QString & comment,
                                  const QString &description)
{
    QTC_ASSERT(!m_pasteReply, return);
    QByteArray data = "api=+xKvWG+1UFXkr2Kn3Td4AnpYtCIjA4qt&";
    data += "content=";
    data += QUrl::toPercentEncoding(fixNewLines(text));
    data += "&type=";
    data += toTypeId(ct);
    data += "&description=";
    data += QUrl::toPercentEncoding(comment);
    data += "&expiry=";
    data += QUrl::toPercentEncoding(QLatin1String(expiryValue(expiryDays)));
    data += "&name="; // Title or name.
    data += QUrl::toPercentEncoding(description);
    // fire request
    const QString link = QLatin1String(internalUrlC) + QLatin1String("quiet-paste.php");
    m_pasteReply = httpPost(link, data);
    connect(m_pasteReply, &QNetworkReply::finished,
            this, &PasteBinDotCaProtocol::pasteFinished);
}

void PasteBinDotCaProtocol::pasteFinished()
{
    if (m_pasteReply->error()) {
        qWarning("%s protocol error: %s", protocolNameC, qPrintable(m_pasteReply->errorString()));
    } else {
        /// returns ""SUCCESS:[id]""
        const QByteArray data = m_pasteReply->readAll();
        const QString link = QString::fromLatin1(urlC) + QString::fromLatin1(data).remove(QLatin1String("SUCCESS:"));
        emit pasteDone(link);
    }
    m_pasteReply->deleteLater();
    m_pasteReply = 0;
}

void PasteBinDotCaProtocol::fetchFinished()
{
    QString title;
    QString content;
    bool error = m_fetchReply->error();
    if (error) {
        content = m_fetchReply->errorString();
    } else {
        title = name() + QLatin1String(": ") + m_fetchId;
        const QByteArray data = m_fetchReply->readAll();
        content = QString::fromUtf8(data);
        content.remove(QLatin1Char('\r'));
    }
    m_fetchReply->deleteLater();
    m_fetchReply = 0;
    emit fetchDone(title, content, error);
}

void PasteBinDotCaProtocol::list()
{
    QTC_ASSERT(!m_listReply, return);
    m_listReply = httpGet(QLatin1String(urlC));
    connect(m_listReply, &QNetworkReply::finished, this, &PasteBinDotCaProtocol::listFinished);
}

bool PasteBinDotCaProtocol::checkConfiguration(QString *errorMessage)
{
    if (m_hostChecked) // Check the host once.
        return true;
    const bool ok = httpStatus(QLatin1String(urlC), errorMessage);
    if (ok)
        m_hostChecked = true;
    return ok;
}

/* Quick & dirty: Parse page does no more work due to internal javascript/websocket magic - so,
 * search for _initial_ json array containing the last added pastes.
\code
<script type="text/javascript">var pHistoryInitial = [{"id":3791300,"ts":1491288268,"name":"try",
"expires":1491374668},
\endcode */

static inline QStringList parseLists(QIODevice *io)
{
    QStringList rc;

    QByteArray data = io->readAll();
    const QByteArray history("<script type=\"text/javascript\">var pHistoryInitial = ");
    int pos = data.indexOf(history);
    if (pos == -1)
        return rc;
    data.remove(0, pos + history.size());
    pos = data.indexOf(";</script>");
    if (pos == -1)
        return rc;
    data.truncate(pos);
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
        return rc;
    QJsonArray array = doc.array();
    for (const QJsonValue &val : array) {
        const QJsonObject obj = val.toObject();
        const QJsonValue id = obj.value("id");
        const QJsonValue name = obj.value("name");
        if (!id.isUndefined())
            rc.append(QString::number(id.toInt()) + ' ' + name.toString());
    }
    return rc;
}

void PasteBinDotCaProtocol::listFinished()
{
    const bool error = m_listReply->error();
    if (error)
        qWarning("%s list failed: %s", protocolNameC, qPrintable(m_listReply->errorString()));
    else
        emit listDone(name(), parseLists(m_listReply));
    m_listReply->deleteLater();
    m_listReply = nullptr;
}

} // namespace CodePaster
