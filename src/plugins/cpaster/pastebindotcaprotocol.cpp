/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "pastebindotcaprotocol.h"

#include <utils/qtcassert.h>

#include <QtNetwork/QNetworkReply>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamAttributes>
#include <QtCore/QStringList>

static const char urlC[] = "http://pastebin.ca/";

namespace CodePaster {
PasteBinDotCaProtocol::PasteBinDotCaProtocol(const NetworkAccessManagerProxyPtr &nw) :
    NetworkProtocol(nw),
    m_fetchReply(0),
    m_listReply(0),
    m_pasteReply(0)
{
}

unsigned PasteBinDotCaProtocol::capabilities() const
{
    return ListCapability | PostDescriptionCapability;
}

void PasteBinDotCaProtocol::fetch(const QString &id)
{
    QTC_ASSERT(!m_fetchReply, return)
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
    connect(m_fetchReply, SIGNAL(finished()), this, SLOT(fetchFinished()));
    m_fetchId = id;
}

void PasteBinDotCaProtocol::paste(const QString &text,
                                  ContentType /* ct */,
                                  const QString &username,
                                  const QString & /* comment */,
                                  const QString &description)
{
    QTC_ASSERT(!m_pasteReply, return)
    QByteArray data = "content=";
    data += QUrl::toPercentEncoding(fixNewLines(text));
    data += "&description=";
    data += QUrl::toPercentEncoding(description);
    data += "&type=1&expiry=1%20day&name=";
    data += QUrl::toPercentEncoding(username);
    // fire request
    const QString link = QLatin1String(urlC) + QLatin1String("quiet-paste.php");
    m_pasteReply = httpPost(link, data);
    connect(m_pasteReply, SIGNAL(finished()), this, SLOT(pasteFinished()));
}

void PasteBinDotCaProtocol::pasteFinished()
{
    if (m_pasteReply->error()) {
        qWarning("Pastebin.ca protocol error: %s", qPrintable(m_pasteReply->errorString()));
    } else {
        /// returns ""SUCCESS:[id]""
        const QByteArray data = m_pasteReply->readAll();
        const QString link = QString::fromLatin1(urlC) + QString::fromAscii(data).remove(QLatin1String("SUCCESS:"));
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
        title = QString::fromLatin1("Pastebin.ca: %1").arg(m_fetchId);
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
    connect(m_listReply, SIGNAL(finished()), this, SLOT(listFinished()));
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
        switch(reader.readNext()) {
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
    if (error) {
        qWarning("pastebin.ca list failed: %s", qPrintable(m_listReply->errorString()));
    } else {
        emit listDone(name(), parseLists(m_listReply));
    }
    m_listReply->deleteLater();
    m_listReply = 0;
}

} // namespace CodePaster
