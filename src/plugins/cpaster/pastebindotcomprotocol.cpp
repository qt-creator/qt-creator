/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
#include <QtCore/QByteArray>

#include <QtNetwork/QNetworkReply>

enum { debug = 0 };

static const char pastePhpScriptpC[] = "api_public.php";
static const char fetchPhpScriptpC[] = "raw.php";

namespace CodePaster {
PasteBinDotComProtocol::PasteBinDotComProtocol(const NetworkAccessManagerProxyPtr &nw) :
    NetworkProtocol(nw),
    m_settings(new PasteBinDotComSettings),
    m_fetchReply(0),
    m_pasteReply(0),
    m_listReply(0),
    m_fetchId(-1),
    m_postId(-1),
    m_hostChecked(false)
{
}

QString PasteBinDotComProtocol::protocolName()
{
    return QLatin1String("Pastebin.Com");
}

unsigned PasteBinDotComProtocol::capabilities() const
{
    return ListCapability;
}

bool PasteBinDotComProtocol::checkConfiguration(QString *errorMessage)
{
    if (m_hostChecked)  // Check the host once.
        return true;
    const bool ok = httpStatus(hostName(false), errorMessage);
    if (ok)
        m_hostChecked = true;
    return ok;
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
        return "paste_format=diff"; // v3.X 'dff' -> 'diff'
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
    QByteArray pasteData = format(ct);
    if (!pasteData.isEmpty())
        pasteData.append('&');
    pasteData += "paste_name=";
    pasteData += QUrl::toPercentEncoding(username);

    const QString subDomain = m_settings->hostPrefix();
    if (!subDomain.isEmpty()) {
        pasteData += "&paste_subdomain=";
        pasteData += QUrl::toPercentEncoding(subDomain);
    }

    pasteData += "&paste_code=";
    pasteData += QUrl::toPercentEncoding(fixNewLines(text));

    // fire request
    QString link;
    QTextStream(&link) << "http://" << hostName(false) << '/' << pastePhpScriptpC;

    m_pasteReply = httpPost(link, pasteData);
    connect(m_pasteReply, SIGNAL(finished()), this, SLOT(pasteFinished()));
    if (debug)
        qDebug() << "paste: sending " << m_pasteReply << link << pasteData;
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

    m_fetchReply = httpGet(link);
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
    const QString url = QLatin1String("http://") + hostName(true) + QLatin1String("/archive");
    m_listReply = httpGet(url);
    connect(m_listReply, SIGNAL(finished()), this, SLOT(listFinished()));
    if (debug)
        qDebug() << "list: sending " << url << m_listReply;
}

static inline void padString(QString *s, int len)
{
    const int missing = len - s->length();
    if (missing > 0)
        s->append(QString(missing, QLatin1Char(' ')));
}

/* Quick & dirty: Parse out the 'archive' table as of 16.3.2011:
\code
 <table class="maintable" cellspacing="0">
   <tr class="top">
    <th scope="col" align="left">Name / Title</th>
    <th scope="col" align="left">Posted</th>
    <th scope="col" align="left">Expires</th>
    <th scope="col" align="left">Size</th>
    <th scope="col" align="left">Syntax</th>
    <th scope="col" align="left">User</th>
   </tr>
   <tr class="g">
    <td class="icon"><a href="/8ZRqkcaP">Untitled</a></td>
    <td>2 sec ago</td>
    <td>Never</td>
    <td>9.41 KB</td>
    <td><a href="/archive/text">None</a></td>
    <td>a guest</td>
   </tr>
   <tr>
\endcode */

enum ParseState
{
    OutSideTable, WithinTable, WithinTableRow, WithinTableHeaderElement,
    WithinTableElement, WithinTableElementAnchor, ParseError
};

static inline ParseState nextOpeningState(ParseState current, const QStringRef &element)
{
    switch (current) {
    case OutSideTable:
        if (element == QLatin1String("table"))
            return WithinTable;
        return OutSideTable;
    case WithinTable:
        if (element == QLatin1String("tr"))
            return WithinTableRow;
        break;
    case WithinTableRow:
        if (element == QLatin1String("td"))
            return WithinTableElement;
        if (element == QLatin1String("th"))
            return WithinTableHeaderElement;
        break;
    case WithinTableElement:
        if (element == QLatin1String("a"))
            return WithinTableElementAnchor;
        break;
    case WithinTableHeaderElement:
    case WithinTableElementAnchor:
    case ParseError:
        break;
    }
    return ParseError;
}

static inline ParseState nextClosingState(ParseState current, const QStringRef &element)
{
    switch (current) {
    case OutSideTable:
        return OutSideTable;
    case WithinTable:
        if (element == QLatin1String("table"))
            return OutSideTable;
        break;
    case WithinTableRow:
        if (element == QLatin1String("tr"))
            return WithinTable;
        break;
    case WithinTableElement:
        if (element == QLatin1String("td"))
            return WithinTableRow;
        break;
    case WithinTableHeaderElement:
        if (element == QLatin1String("th"))
            return WithinTableRow;
        break;
    case WithinTableElementAnchor:
        if (element == QLatin1String("a"))
            return WithinTableElement;
        break;
    case ParseError:
        break;
    }
    return ParseError;
}

static inline QStringList parseLists(QIODevice *io)
{
    enum { maxEntries = 200 }; // Limit the archive, which can grow quite large.

    QStringList rc;
    QXmlStreamReader reader(io);
    ParseState state = OutSideTable;
    int tableRow = 0;
    int tableColumn = 0;

    const QString hrefAttribute = QLatin1String("href");

    QString link;
    QString user;
    QString description;

    while (!reader.atEnd()) {
        switch(reader.readNext()) {
        case QXmlStreamReader::StartElement:
            state = nextOpeningState(state, reader.name());
            switch (state) {
            case WithinTableRow:
                tableColumn = 0;
                break;
            case OutSideTable:
            case WithinTable:
            case WithinTableHeaderElement:
            case WithinTableElement:
                break;
            case WithinTableElementAnchor: // 'href="/svb5K8wS"'
                if (tableColumn == 0) {
                    link = reader.attributes().value(hrefAttribute).toString();
                    if (link.startsWith(QLatin1Char('/')))
                        link.remove(0, 1);
                }
                break;
            case ParseError:
                return rc;
            } // switch startelement state
            break;
        case QXmlStreamReader::EndElement:
            state = nextClosingState(state, reader.name());
            switch (state) {
            case OutSideTable:
                if (tableRow) // Seen the table, bye.
                    return rc;
                break;
            case WithinTable:
                if (tableRow && !user.isEmpty() && !link.isEmpty() && !description.isEmpty()) {
                    QString entry = link;
                    entry += QLatin1Char(' ');
                    entry += user;
                    entry += QLatin1Char(' ');
                    entry += description;
                    rc.push_back(entry);
                    if (rc.size() >= maxEntries)
                        return rc;
                }
                tableRow++;
                user.clear();
                link.clear();
                description.clear();
                break;
            case WithinTableRow:
                tableColumn++;
                break;
            case WithinTableHeaderElement:
            case WithinTableElement:
            case WithinTableElementAnchor:
                break;
            case ParseError:
                return rc;
            } // switch endelement state
            break;
        case QXmlStreamReader::Characters:
            switch (state) {
                break;
            case WithinTableElement:
                if (tableColumn == 5)
                    user = reader.text().toString();
                break;
            case WithinTableElementAnchor:
                if (tableColumn == 0)
                    description = reader.text().toString();
                break;
            default:
                break;
            } // switch characters read state
            break;
       default:
            break;
        } // switch reader state
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

Core::IOptionsPage *PasteBinDotComProtocol::settingsPage() const
{
    return m_settings;
}
} // namespace CodePaster
