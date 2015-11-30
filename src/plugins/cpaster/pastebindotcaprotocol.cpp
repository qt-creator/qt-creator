/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pastebindotcaprotocol.h"

#include <utils/qtcassert.h>

#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QStringList>

static const char urlC[] = "http://pastebin.ca/";
static const char internalUrlC[] = "http://pbin.ca/";
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
    const QString url = QLatin1String(urlC);
    const QString rawPostFix = QLatin1String("raw/");
    // Create link as ""http://pastebin.ca/raw/[id]"
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

/* Quick & dirty: Parse the <div>-elements with the "Recent Posts" listing
 * out of the page.
\code
<div class="menutitle"><h2>Recent Posts</h2></div>
    <div class="items" id="idmenurecent-collapse">
        <div class='recentlink'>
            <a href="/[id]" class="rjt" rel="/preview.php?id=[id]">[nameTitle]</a>
            <div class='recentdetail'>[time spec]</div>
        </div>
 ...<h2>Create a New Pastebin Post</h2>
\endcode */

static inline QStringList parseLists(QIODevice *io)
{
    enum State { OutsideRecentLink, InsideRecentLink };

    QStringList rc;

    const QString classAttribute = QLatin1String("class");
    const QString divElement = QLatin1String("div");
    const QString anchorElement = QLatin1String("a");

    // Start parsing at the 'recent posts' entry as the HTML above is not well-formed
    // as of 8.4.2010. This will then terminate with an error.
    QByteArray data = io->readAll();
    const QByteArray recentPosts("<h2>Recent Posts</h2></div>");
    const int recentPostsPos = data.indexOf(recentPosts);
    if (recentPostsPos == -1)
        return rc;
    data.remove(0, recentPostsPos + recentPosts.size());
    QXmlStreamReader reader(data);
    State state = OutsideRecentLink;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            // Inside a <div> of an entry: Anchor or description
            if (state == InsideRecentLink && reader.name() == anchorElement) { // Anchor
                // Strip host from link
                QString link = reader.attributes().value(QLatin1String("href")).toString();
                if (link.startsWith(QLatin1Char('/')))
                    link.remove(0, 1);
                const QString nameTitle = reader.readElementText();
                rc.push_back(link + QLatin1Char(' ') + nameTitle);
            } else if (state == OutsideRecentLink && reader.name() == divElement) { // "<div>" state switching
                if (reader.attributes().value(classAttribute) == QLatin1String("recentlink"))
                    state = InsideRecentLink;
            } // divElement
            break;
       default:
            break;
        } // switch reader
    } // while reader.atEnd()
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
    m_listReply = 0;
}

} // namespace CodePaster
