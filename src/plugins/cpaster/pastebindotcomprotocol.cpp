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

#include "pastebindotcomprotocol.h"
#include "pastebindotcomsettings.h"

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamAttributes>

#include <QtNetwork/QNetworkReply>

enum { debug = 0 };

static const char pastePhpScriptpC[] = "api_public.php";
static const char fetchPhpScriptpC[] = "raw.php";

namespace CodePaster {
PasteBinDotComProtocol::PasteBinDotComProtocol() :
    m_settings(new PasteBinDotComSettings),
    m_fetchReply(0),
    m_pasteReply(0),
    m_listReply(0),
    m_fetchId(-1),
    m_postId(-1)
{
}

unsigned PasteBinDotComProtocol::capabilities() const
{
    return ListCapability;
}

QString PasteBinDotComProtocol::hostName(bool withSubDomain) const
{

    QString rc;
    if (withSubDomain) {
        rc = m_settings->hostPrefix();
        if (!rc.isEmpty())
            rc.append(QLatin1Char('.'));
    }
    rc.append(QLatin1String("pastebin.com"));
    return rc;
}

static inline QByteArray format(Protocol::ContentType ct)
{
    switch (ct) {
    case Protocol::Text:
        break;
    case Protocol::C:
        return "paste_format=cpp";
        break;
    case Protocol::JavaScript:
        return "paste_format=javascript";
        break;
    case Protocol::Diff:
        return "paste_format=dff";
        break;
    case Protocol::Xml:
        return "paste_format=xml";
        break;
    }
    return QByteArray();
}

void PasteBinDotComProtocol::paste(const QString &text,
                                   ContentType ct,
                                   const QString &username,
                                   const QString & /* comment */,
                                   const QString & /* description */)
{
    QTC_ASSERT(!m_pasteReply, return;)

    // Format body
    m_pasteData = format(ct);
    if (!m_pasteData.isEmpty())
        m_pasteData.append('&');
    m_pasteData += "paste_name=";
    m_pasteData += QUrl::toPercentEncoding(username);

    const QString subDomain = m_settings->hostPrefix();
    if (!subDomain.isEmpty()) {
        m_pasteData += "&paste_subdomain=";
        m_pasteData += QUrl::toPercentEncoding(subDomain);
    }

    m_pasteData += "&paste_code=";
    m_pasteData += QUrl::toPercentEncoding(fixNewLines(text));

    // fire request
    QString link;
    QTextStream(&link) << "http://" << hostName(false) << '/' << pastePhpScriptpC;

    QUrl url(link);
    QNetworkRequest r(url);
    m_pasteReply = m_manager.post(r, m_pasteData);
    connect(m_pasteReply, SIGNAL(finished()), this, SLOT(pasteFinished()));
    if (debug)
        qDebug() << "paste: sending " << m_pasteReply << link << m_pasteData;
}

void PasteBinDotComProtocol::pasteFinished()
{
    if (m_pasteReply->error()) {
        qWarning("Pastebin.com protocol error: %s", qPrintable(m_pasteReply->errorString()));
    } else {
        emit pasteDone(QString::fromAscii(m_pasteReply->readAll()));
    }

    m_pasteReply->deleteLater();
    m_pasteReply = 0;
}

void PasteBinDotComProtocol::fetch(const QString &id)
{
    const QString httpProtocolPrefix = QLatin1String("http://");

    QTC_ASSERT(!m_fetchReply, return;)

    // Did we get a complete URL or just an id. Insert a call to the php-script
    QString link;
    if (id.startsWith(httpProtocolPrefix)) {
        // Change "http://host/id" -> "http://host/script?i=id".
        const int lastSlashPos = id.lastIndexOf(QLatin1Char('/'));
        link = id.mid(0, lastSlashPos);
        QTextStream(&link) << '/' << fetchPhpScriptpC<< "?i=" << id.mid(lastSlashPos + 1);
    } else {
        // format "http://host/script?i=id".
        QTextStream(&link) << "http://" << hostName(true) << '/' << fetchPhpScriptpC<< "?i=" << id;
    }

    if (debug)
        qDebug() << "fetch: sending " << link;
    QUrl url(link);
    QNetworkRequest r(url);

    m_fetchReply = m_manager.get(r);
    connect(m_fetchReply, SIGNAL(finished()), this, SLOT(fetchFinished()));
    m_fetchId = id;
}

void PasteBinDotComProtocol::fetchFinished()
{
    QString title;
    QString content;
    const bool error = m_fetchReply->error();
    if (error) {
        content = m_fetchReply->errorString();
        if (debug)
            qDebug() << "fetchFinished: error" << m_fetchId << content;
    } else {
        title = QString::fromLatin1("Pastebin.com: %1").arg(m_fetchId);
        content = QString::fromAscii(m_fetchReply->readAll());
        // Cut out from '<pre>' formatting
        const int preEnd = content.lastIndexOf(QLatin1String("</pre>"));
        if (preEnd != -1)
            content.truncate(preEnd);
        const int preStart = content.indexOf(QLatin1String("<pre>"));
        if (preStart != -1)
            content.remove(0, preStart + 5);
        // Make applicable as patch.
        content = Protocol::textFromHtml(content);
        content += QLatin1Char('\n');
        // -------------
        if (debug) {
            QDebug nsp = qDebug().nospace();
            nsp << "fetchFinished: " << content.size() << " Bytes";
            if (debug > 1)
                nsp << content;
        }
    }
    m_fetchReply->deleteLater();
    m_fetchReply = 0;
    emit fetchDone(title, content, error);
}

void PasteBinDotComProtocol::list()
{
    QTC_ASSERT(!m_listReply, return;)

    // fire request
    QUrl url(QLatin1String("http://") + hostName(true));
    QNetworkRequest r(url);
    m_listReply = m_manager.get(r);
    connect(m_listReply, SIGNAL(finished()), this, SLOT(listFinished()));
    if (debug)
        qDebug() << "list: sending " << m_listReply;
}

static inline void padString(QString *s, int len)
{
    const int missing = len - s->length();
    if (missing > 0)
        s->append(QString(missing, QLatin1Char(' ')));
}

/* Quick & dirty: Parse the <div>-elements with the "Recent Posts" listing
 * out of the page.
\code
<div class="content_left_title">Recent Posts</div>
    <div class="content_left_box">
        <div class="clb_top"><a href="http://pastebin.com/id">User</a></div>
        <div class="clb_bottom"><span>Title</div>
\endcode */

static inline QStringList parseLists(QIODevice *io)
{
    enum State { OutsideRecentPostList, InsideRecentPostBox, InsideRecentPost };

    QStringList rc;
    QXmlStreamReader reader(io);

    const QString classAttribute = QLatin1String("class");
    const QString divElement = QLatin1String("div");
    const QString anchorElement = QLatin1String("a");
    const QString spanElement = QLatin1String("span");
    State state = OutsideRecentPostList;
    while (!reader.atEnd()) {
        switch(reader.readNext()) {
        case QXmlStreamReader::StartElement:
            // Inside a <div> of an entry: Anchor or description
            if (state == InsideRecentPost) {
                if (reader.name() == anchorElement) { // Anchor
                    // Strip host from link
                    QString link = reader.attributes().value(QLatin1String("href")).toString();
                    const int slashPos = link.lastIndexOf(QLatin1Char('/'));
                    if (slashPos != -1)
                        link.remove(0, slashPos + 1);
                    const QString user = reader.readElementText();
                    rc.push_back(link + QLatin1Char(' ') + user);
                } else if (reader.name() == spanElement) { // <span> with description
                    const QString description = reader.readElementText();
                    QTC_ASSERT(!rc.isEmpty(), return rc; )
                    padString(&rc.back(), 25);
                    rc.back() += QLatin1Char(' ');
                    rc.back() += description;
                }
            } else if (reader.name() == divElement) { // "<div>" state switching
                switch (state) {
                case OutsideRecentPostList:
                    if (reader.attributes().value(classAttribute) == QLatin1String("content_left_box"))
                        state = InsideRecentPostBox;
                    break;
                case InsideRecentPostBox:
                    state = InsideRecentPost;
                    break;
                default:
                    break;
                }
            } // divElement
            break;
        case QXmlStreamReader::EndElement:
            if (reader.name() == divElement) {
                switch (state) {
                case InsideRecentPost:
                    state = InsideRecentPostBox;
                    break;
                case InsideRecentPostBox: // Stop parsing  when leaving the box.
                    return rc;
                    break;
                default:
                    break;
                }
            }
            break;
       default:
            break;
        }
    }
    return rc;
}

void PasteBinDotComProtocol::listFinished()
{
    const bool error = m_listReply->error();
    if (error) {
        if (debug)
            qDebug() << "listFinished: error" << m_listReply->errorString();
    } else {
        QStringList list = parseLists(m_listReply);
        emit listDone(name(), list);
        if (debug)
            qDebug() << list;
    }
    m_listReply->deleteLater();
    m_listReply = 0;
}

Core::IOptionsPage *PasteBinDotComProtocol::settingsPage()
{
    return m_settings;
}
} // namespace CodePaster
